#ifndef _BASIC_RECEIVER_HPP_
#define _BASIC_RECEIVER_HPP_

#include "./BasicRole.h"
#include "./UDPFileWriter.h"

namespace my
{
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

    protected:
        float m_send_ack_loss = 0.0f;
        float m_recv_loss = 0.0f;

        void sendAckToPeer(char ack_num);
        UDPDataframe recvUDPDataframeFromPeer();
    };

    template <int receiverWindowSize, int seqNumBound>
    BasicReceiver<receiverWindowSize, seqNumBound>::~BasicReceiver() {}

    template <int receiverWindowSize, int seqNumBound>
    void BasicReceiver<receiverWindowSize, seqNumBound>::sendAckToPeer(char ack_num)
    {
        if (random() < m_send_ack_loss) {
            pretty_log << ::std::format("Loss event occurs, ack frame {} was not sent", (int)ack_num);
            return;
        }
        sendAckTo(ack_num, this->m_host, this->m_peer);
    }

    template <int receiverWindowSize, int seqNumBound>
    UDPDataframe BasicReceiver<receiverWindowSize, seqNumBound>::recvUDPDataframeFromPeer()
    {
        UDPDataframe dataframe;
        Peer peer;
        while (true) {
            do {
                dataframe = ::std::move(recvUDPDataframeFrom(this->m_host, peer));
            } while (!dataframe.isData() || this->m_peer != peer);

            if (random() >= m_recv_loss) {
                break;
            }

            pretty_log << ::std::format("Loss event occurs, data frame {} was not received (already sent by peer)", (int)dataframe.getDataNum());
        }
        return dataframe;
    }
} // namespace my

#endif // _BASIC_RECEIVER_HPP_