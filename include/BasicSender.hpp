#ifndef _BASIC_SENDER_HPP_
#define _BASIC_SENDER_HPP_

#include "./BasicRole.h"
#include "./UDPFileReader.h"

namespace my
{
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

    protected:
        float m_send_loss = 0.0f;
        float m_recv_ack_loss = 0.0f;

        int recvAckFromPeer();
        void sendUDPDataframeToPeer(UDPFileReader &reader, int index);
    };

    template <int senderWindowSize, int seqNumBound>
    BasicSender<senderWindowSize, seqNumBound>::~BasicSender() {}

    template <int senderWindowSize, int seqNumBound>
    int BasicSender<senderWindowSize, seqNumBound>::recvAckFromPeer()
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(this->m_host.getSocket(), &readfds);
        TIMEVAL timeout = {0, 100000}; // 0.1s

        int sum = select(0, &readfds, nullptr, nullptr, &timeout);
        if (sum == SOCKET_ERROR) {
            pretty_out << ::std::format("throw from BasicSender::recvAckFromPeer(): select() failed, WSAGetLastError() = {0}", WSAGetLastError());
            throw std::runtime_error("select() failed");
        } else if (sum == 0) {
            return -1;
        }

        Peer peer;
        char ack_num;
        try {
            ack_num = recvAckFrom(this->m_host, peer);
        } catch (const std::runtime_error &e) {
            pretty_out
                << ::std::format("catch by BasicSender::recvAckFromPeer():")
                << e.what();
            return -1;
        }

        if (peer == this->m_peer) {
            if (this->random() < this->m_recv_ack_loss) {
                pretty_log << ::std::format("Loss event occurs, ack frame {} was not received (already sent by peer)", (int)ack_num);
                return -1;
            }
            return ack_num;
        } else {
            return -1;
        }
    }

    template <int senderWindowSize, int seqNumBound>
    inline void BasicSender<senderWindowSize, seqNumBound>::sendUDPDataframeToPeer(UDPFileReader &reader, int index)
    {
        if (this->random() < this->m_send_loss) {
            pretty_log_con << ::std::format("Loss event occurs, data frame {} was not sent", index);
            return;
        }
        UDPDataframe dataframe = reader.getDataframe(index);
        dataframe.setDataNum(index % seqNumBound);
        sendUDPDataframeTo(dataframe, this->m_host, this->m_peer);
    }
} // namespace my

#endif // _BASIC_SENDER_HPP_