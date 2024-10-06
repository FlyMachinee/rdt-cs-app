#ifndef _GBN_PROTOCOL_HPP_
#define _GBN_PROTOCOL_HPP_

#include "./BasicReceiver.hpp"
#include "./BasicSender.hpp"
#include "./Timer.hpp"

namespace my
{
    template <int senderWindowSize, int seqNumBound>
        requires(senderWindowSize <= seqNumBound - 1 && senderWindowSize > 0)
    class GBN_Sender : public BasicSender<senderWindowSize, seqNumBound>
    {
    public:
        GBN_Sender() = default;
        GBN_Sender(SOCKET host_socket) : BasicSender<senderWindowSize, seqNumBound>(host_socket) {};
        virtual ~GBN_Sender() = default;

        GBN_Sender(const GBN_Sender &) = delete;
        GBN_Sender &operator=(const GBN_Sender &) = delete;

        virtual void sendtoPeer(::std::string_view filename) override;

    private:
        Timer m_timer;
    };

    template <int seqNumBound>
        requires(seqNumBound >= 2)
    class GBN_Receiver : public BasicReceiver<1, seqNumBound>
    {
    public:
        GBN_Receiver() = default;
        GBN_Receiver(SOCKET host_socket) : BasicReceiver<1, seqNumBound>(host_socket) {};
        virtual ~GBN_Receiver() = default;

        GBN_Receiver(const GBN_Receiver &) = delete;
        GBN_Receiver &operator=(const GBN_Receiver &) = delete;

        virtual void recvfromPeer(::std::string_view file_path) override;
    };

    template <int senderWindowSize, int seqNumBound>
        requires(senderWindowSize <= seqNumBound - 1 && senderWindowSize > 0)
    class GBN_Transceiver : public GBN_Sender<senderWindowSize, seqNumBound>,
                            public GBN_Receiver<seqNumBound>
    {
    public:
        GBN_Transceiver() = default;
        GBN_Transceiver(SOCKET host_socket) : BasicRole(host_socket) {};
        virtual ~GBN_Transceiver() = default;
    };

    template <int senderWindowSize, int seqNumBound>
        requires(senderWindowSize <= seqNumBound - 1 && senderWindowSize > 0)
    void my::GBN_Sender<senderWindowSize, seqNumBound>::sendtoPeer(::std::string_view filename)
    {
        UDPFileReader reader(filename);

        int base = 0;
        int next_num = 0;
        int ack_num = -1;
        const int block_count = reader.getBlockCount();
        constexpr int N = senderWindowSize;
        constexpr int M = seqNumBound;

        m_timer.stop();

        while (base <= block_count) {
            // 发送数据帧
            while (next_num < base + N && next_num <= block_count) {
                pretty_log << ::std::format("Send data frame {}({}/{})", next_num % M, next_num, block_count);
                this->sendUDPDataframeToPeer(reader, next_num);

                // 如果是窗口第一个数据帧，启动定时器
                if (base == next_num) {
                    m_timer.setTimeout(this->m_timeout);
                }
                ++next_num;
            }

            // 接收确认帧
            while ((ack_num = this->recvAckFromPeer()) != -1) {
                int actual_ack_num = getActualForwardBlockNum(base, ack_num, M);

                if (actual_ack_num >= base + N || actual_ack_num > block_count) {
                    // 确认号超出窗口范围，丢弃
                    // 用于处理pkt0没有收到，但是pkt1已经收到的情况
                    // 这时对方会发送一个超出窗口范围的ack
                    pretty_log << ::std::format("Receive ack frame {}({}/{}), ignored", ack_num, getActualBackwardBlockNum(base, ack_num, M), block_count);
                    continue;
                }

                pretty_log << ::std::format("Receive ack frame {}({}/{})", ack_num, actual_ack_num, block_count);

                // 累计确认
                base = actual_ack_num + 1;

                if (base == next_num) {
                    // 如果窗口已空，停止定时器
                    m_timer.stop();
                } else {
                    // 否则重置定时器
                    m_timer.setTimeout(this->m_timeout);
                }
            }

            // 超时重传
            if (m_timer.isTimeout()) {
                pretty_log << "Timeout, resend all data frames";

                // 重传窗口内的所有数据帧
                for (int i = base; i < next_num; i++) {
                    pretty_log_con << ::std::format("Resend data frame {}({}/{})", i % M, i, block_count);

                    this->sendUDPDataframeToPeer(reader, i);
                }
                m_timer.setTimeout(this->m_timeout);
            }
        }
    }

    template <int seqNumBound>
        requires(seqNumBound >= 2)
    void my::GBN_Receiver<seqNumBound>::recvfromPeer(::std::string_view file_path)
    {
        UDPFileWriter writer(file_path);

        // 表示已接收到的最大数据帧序号
        // 设为-1以处理第0个数据帧没有收到的情况
        // 这时对方会发送一个超出窗口范围的ack
        int base = -1;
        constexpr int M = seqNumBound;

        bool receive_end = false;
        // 阻塞接收数据帧
        while (!receive_end) {
            UDPDataframe dataframe = this->recvUDPDataframeFromPeer();

            int data_num = dataframe.getDataNum();
            int length;
            dataframe.data(length);

            int actual_forward_block_num = getActualForwardBlockNum(base, data_num, M);

            if (base + 1 == actual_forward_block_num) {
                // 期望的数据帧，顺序接收
                pretty_log << ::std::format("Receive data frame {}({})", data_num, actual_forward_block_num);

                if (length == 0) {
                    // 空的结束帧
                    pretty_log << "End frame";
                    receive_end = true;
                } else {
                    writer.append(dataframe);
                }
                base++;
            } else {
                // 乱序到达则丢弃
                pretty_log << ::std::format("Receive data frame {}({}), discard", data_num, actual_forward_block_num);
            }

            // 发送确认帧
            pretty_log_con << ::std::format("Send ack frame {}({})", (base + M) % M, base);
            this->sendAckToPeer(base % M);
        }
    }

} // namespace my

#endif // _GBN_PROTOCOL_HPP_