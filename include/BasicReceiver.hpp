#ifndef _BASIC_RECEIVER_HPP_
#define _BASIC_RECEIVER_HPP_

#include "./BasicRole.h"
#include "./UDPFileWriter.h"

namespace my
{

    /**
     * @brief BasicReceiver是一个抽象类，它是所有接收方的基类
     * @tparam receiverWindowSize 接收窗口大小
     * @tparam seqNumBound 序列号的上界
     * @details 封装了接收方用于的接收数据分组、发送ACK的方法
     */
    template <int receiverWindowSize, int seqNumBound>
    class BasicReceiver : virtual public BasicRole
    {
    public:
        BasicReceiver() = default;
        BasicReceiver(SOCKET host_socket) : BasicRole(host_socket) {};
        virtual ~BasicReceiver() = 0;
        BasicReceiver(const BasicReceiver &) = delete;
        BasicReceiver &operator=(const BasicReceiver &) = delete;

        virtual void recvfromPeer(::std::string_view file_path) = 0;

        void setSendAckLoss(float loss) noexcept { m_send_ack_loss = loss; }
        void setRecvLoss(float loss) noexcept { m_recv_loss = loss; }
        float getSendAckLoss() const noexcept { return m_send_ack_loss; }
        float getRecvLoss() const noexcept { return m_recv_loss; }

        void enableReceiverLoss() noexcept { m_enable_loss = true; }
        void disableReceiverLoss() noexcept { m_enable_loss = false; }

    protected:
        float m_send_ack_loss = 0.0f;
        float m_recv_loss = 0.0f;
        bool m_enable_loss = false;

        void sendAckToPeer(unsigned char ack_num);
        UDPDataframe recvUDPDataFromPeer();

    private:
    };

    template <int receiverWindowSize, int seqNumBound>
    BasicReceiver<receiverWindowSize, seqNumBound>::~BasicReceiver() {}

    /**
     * @brief 发送ACK到对等方
     * @param ack_num 确认号
     */
    template <int receiverWindowSize, int seqNumBound>
    void BasicReceiver<receiverWindowSize, seqNumBound>::sendAckToPeer(unsigned char ack_num)
    {
        // 在接收方发送ACK时，进行丢包模拟
        if (m_enable_loss && random() < m_send_ack_loss) {
            pretty_log_con << ::std::format("Loss event occurs, ack frame {} was not sent", (int)ack_num);
            return;
        }
        sendAckTo(ack_num, this->m_host, this->m_peer);
    }

    /**
     * @brief 阻塞地从对等方接收UDP数据分组
     * @return 接收到的UDP数据分组
     */
    template <int receiverWindowSize, int seqNumBound>
    UDPDataframe BasicReceiver<receiverWindowSize, seqNumBound>::recvUDPDataFromPeer()
    {
        UDPDataframe dataframe;
        Peer peer;
        while (true) {
            do {
                dataframe = ::std::move(recvUDPDataframeFrom(this->m_host, peer));

                // 需要检查数据分组是否是数据分组，以及是否是从指定对方主机接收到的数据
            } while (!dataframe.isData() || this->m_peer != peer);

            if (!m_enable_loss || random() >= m_recv_loss) {
                break;
            }

            // 在接收方接收数据时，进行丢包模拟
            pretty_log << ::std::format("Loss event occurs, data frame {} was not received (already sent by peer)", (int)dataframe.getDataNum());
        }
        return dataframe;
    }
} // namespace my

#endif // _BASIC_RECEIVER_HPP_