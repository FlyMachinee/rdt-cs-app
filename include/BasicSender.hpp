#ifndef _BASIC_SENDER_HPP_
#define _BASIC_SENDER_HPP_

#include "./BasicRole.h"
#include "./UDPFileReader.h"

namespace my
{
    /**
     * @brief BasicSender是一个抽象类，它是所有发送方的基类
     * @tparam senderWindowSize 发送窗口大小
     * @tparam seqNumBound 序列号的上界
     * @details 封装了发送方用于发送数据分组、接收ACK的方法
     */
    template <int senderWindowSize, int seqNumBound>
    class BasicSender : virtual public BasicRole
    {
    public:
        BasicSender() = default;
        BasicSender(SOCKET host_socket) : BasicRole(host_socket) {};
        virtual ~BasicSender() = 0;
        BasicSender(const BasicSender &) = delete;
        BasicSender &operator=(const BasicSender &) = delete;

        virtual void sendtoPeer(::std::string_view filename) = 0;

        void setSendLoss(float loss) noexcept { m_send_loss = loss; }
        void setRecvAckLoss(float loss) noexcept { m_recv_ack_loss = loss; }
        float getSendLoss() const noexcept { return m_send_loss; }
        float getRecvAckLoss() const noexcept { return m_recv_ack_loss; }

        void enableSenderLoss() noexcept { m_enable_loss = true; }
        void disableSenderLoss() noexcept { m_enable_loss = false; }

    protected:
        float m_send_loss = 0.0f;
        float m_recv_ack_loss = 0.0f;
        bool m_enable_loss = false;

        int recvAckFromPeer();
        void sendUDPDataToPeer(UDPFileReader &reader, int index);
    };

    template <int senderWindowSize, int seqNumBound>
    BasicSender<senderWindowSize, seqNumBound>::~BasicSender() {}

    /**
     * @brief 非阻塞地接收对等方的ACK
     * @return ACK号，如果没有接收到则返回-1
     */
    template <int senderWindowSize, int seqNumBound>
    int BasicSender<senderWindowSize, seqNumBound>::recvAckFromPeer()
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(this->m_host.getSocket(), &readfds);
        TIMEVAL timeout = {0, 100000}; // 0.1s

        // 通过select()函数实现非阻塞地接收ACK
        int sum = select(0, &readfds, nullptr, nullptr, &timeout);
        if (sum == SOCKET_ERROR) {
            pretty_out << ::std::format("throw from BasicSender::recvAckFromPeer(): select() failed, WSAGetLastError() = {0}", WSAGetLastError());
            throw std::runtime_error("select() failed");
        } else if (sum == 0) {
            // 在0.1s内没有接收到ACK，返回-1
            return -1;
        }

        Peer peer;
        unsigned char ack_num;
        try {
            ack_num = recvAckFrom(this->m_host, peer);
        } catch (const std::runtime_error &e) {
            pretty_out
                << ::std::format("catch by BasicSender::recvAckFromPeer():")
                << e.what();
            return -1;
        }

        // 确保接收到的ACK是从指定对等方发来的
        if (peer == this->m_peer) {
            // 在发送方接收ACK时，进行丢包模拟
            if (m_enable_loss && this->random() < this->m_recv_ack_loss) {
                pretty_log << ::std::format("Loss event occurs, ack frame {} was not received (already sent by peer)", (int)ack_num);
                return -1;
            }
            return ack_num;
        } else {
            return -1;
        }
    }

    /**
     * @brief 发送数据到对等方
     * @param reader 用于读取数据的UDPFileReader对象
     * @param index 数据块编号
     */
    template <int senderWindowSize, int seqNumBound>
    inline void BasicSender<senderWindowSize, seqNumBound>::sendUDPDataToPeer(UDPFileReader &reader, int index)
    {
        // 在发送方发送数据时，进行丢包模拟
        if (m_enable_loss && this->random() < this->m_send_loss) {
            pretty_log_con << ::std::format("Loss event occurs, data frame {} was not sent", index);
            return;
        }
        UDPDataframe dataframe = reader.getDataframe(index);
        dataframe.setDataNum(index % seqNumBound);
        sendUDPDataframeTo(dataframe, this->m_host, this->m_peer);
    }
} // namespace my

#endif // _BASIC_SENDER_HPP_