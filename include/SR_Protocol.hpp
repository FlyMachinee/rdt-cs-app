#ifndef _SR_PROTOCOL_HPP_
#define _SR_PROTOCOL_HPP_

#include "./BasicReceiver.hpp"
#include "./BasicSender.hpp"
#include "./SpinWindow.hpp"

namespace my
{
    template <int senderWindowSize, int seqNumBound>
        requires(senderWindowSize <= seqNumBound / 2 && senderWindowSize > 0)
    class SR_Sender : public BasicSender<senderWindowSize, seqNumBound>
    {
    public:
        SR_Sender() = default;
        SR_Sender(SOCKET host_socket) : BasicSender<senderWindowSize, seqNumBound>(host_socket) {};
        virtual ~SR_Sender() = default;

        SR_Sender(const SR_Sender &) = delete;
        SR_Sender &operator=(const SR_Sender &) = delete;

        virtual void sendtoPeer(::std::string_view file_path) override;

    private:
        SpinWindowWithTimer<senderWindowSize, seqNumBound> m_spin_timer;
    };

    template <int receiverWindowSize, int seqNumBound>
        requires(receiverWindowSize <= seqNumBound / 2 && receiverWindowSize > 0)
    class SR_Receiver : public BasicReceiver<receiverWindowSize, seqNumBound>
    {
    public:
        SR_Receiver() = default;
        SR_Receiver(SOCKET host_socket) : BasicReceiver<receiverWindowSize, seqNumBound>(host_socket) {};
        virtual ~SR_Receiver() = default;

        SR_Receiver(const SR_Receiver &) = delete;
        SR_Receiver &operator=(const SR_Receiver &) = delete;

        virtual void recvfromPeer(::std::string_view file_path) override;

    private:
        SpinWindowWithCache<receiverWindowSize, seqNumBound, UDPDataframe> m_spin_cache;
    };

    template <int windowSize, int seqNumBound>
        requires(windowSize <= seqNumBound / 2)
    class SR_Transceiver : public SR_Sender<windowSize, seqNumBound>,
                           public SR_Receiver<windowSize, seqNumBound>
    {
    public:
        SR_Transceiver() = default;
        SR_Transceiver(SOCKET host_socket) : BasicRole(host_socket) {};
        virtual ~SR_Transceiver() = default;
    };

    template <int senderWindowSize, int seqNumBound>
        requires(senderWindowSize <= seqNumBound / 2 && senderWindowSize > 0)
    void my::SR_Sender<senderWindowSize, seqNumBound>::sendtoPeer(::std::string_view file_path)
    {
        UDPFileReader reader(file_path);
        m_spin_timer.clear();

        int base = 0;
        int next_seq_num = 0;
        int ack_num = -1;
        int timeout_num = -1;
        const int block_count = reader.getBlockCount();
        constexpr int N = senderWindowSize;
        constexpr int M = seqNumBound;

        while (base <= block_count) {
            // 发送数据帧
            while (next_seq_num < base + N && next_seq_num <= block_count) {
                pretty_log << ::std::format("Send data frame {}({}/{})", next_seq_num % M, next_seq_num, block_count);

                this->sendUDPDataframeToPeer(reader, next_seq_num);
                m_spin_timer.timerSetTimeout(next_seq_num % M, this->m_timeout);
                ++next_seq_num;
            }

            // 接收确认帧
            while ((ack_num = this->recvAckFromPeer()) != -1) {
                int actual_forward_block_num = getActualForwardBlockNum(base, ack_num, M);

                pretty_log << ::std::format("Receive ack frame {}({}/{})", ack_num, actual_forward_block_num, block_count);

                // 如果确认号在当前窗口内
                if (actual_forward_block_num <= base + N) {
                    m_spin_timer.submit(ack_num);
                }
            }
            // 尝试滑动窗口
            base += m_spin_timer.spin();

            // 发送超时的数据帧
            while ((timeout_num = m_spin_timer.whichTimerIsTimeout()) != -1) {
                int actual_timeout_num = getActualForwardBlockNum(base, timeout_num, M);

                pretty_log << ::std::format("Timeout for ack frame {}({}/{})", timeout_num, actual_timeout_num, block_count);
                pretty_log << ::std::format("Resend data frame {}({}/{})", timeout_num, actual_timeout_num, block_count);

                this->sendUDPDataframeToPeer(reader, actual_timeout_num);
                m_spin_timer.timerSetTimeout(timeout_num, this->m_timeout);
            }
        }
    }

    template <int receiverWindowSize, int seqNumBound>
        requires(receiverWindowSize <= seqNumBound / 2 && receiverWindowSize > 0)
    void SR_Receiver<receiverWindowSize, seqNumBound>::recvfromPeer(::std::string_view file_path)
    {
        UDPFileWriter writer(file_path);
        m_spin_cache.clear();

        int base = 0;
        constexpr int N = receiverWindowSize;
        constexpr int M = seqNumBound;

        bool receive_end = false;
        int target_block_cnt = 0;

        // 阻塞接收数据帧
        while (!receive_end || base < target_block_cnt) {
            UDPDataframe dataframe = this->recvUDPDataframeFromPeer();

            int seq_num = dataframe.getDataNum();
            int length;
            dataframe.data(length);
            int actual_forward_block_num = getActualForwardBlockNum(base, seq_num, M);
            int actual_backward_block_num = getActualBackwardBlockNum(base, seq_num, M);

            bool in_current_window = actual_forward_block_num < base + N;
            bool in_last_window = actual_backward_block_num >= base - N;
            if (in_current_window && in_last_window) {
                // case not exist
                pretty_err << "Ambiguous block number caught from receiver's perspective";
                ::std::terminate();
            }

            if (in_current_window || in_last_window) {
                if (in_current_window) {
                    // 期望的数据帧，接收或缓存
                    pretty_log << ::std::format("Receive data frame {}({})", seq_num, actual_forward_block_num);

                    if (length == 0) {
                        // 空的结束帧，置标记位，等待接收结束
                        receive_end = true;
                        target_block_cnt = actual_forward_block_num;

                        pretty_log << "End frame";
                    } else {
                        if (m_spin_cache.submit(seq_num, ::std::move(dataframe))) {
                            int cnt = m_spin_cache.spin(writer);
                            base += cnt;
                            if (cnt) {
                                pretty_log << ::std::format("Submit {} data frame(s) to writer", cnt);
                            } else {
                                pretty_log << "Cached";
                            }
                        } else {
                            // 当前窗口的重复的数据帧，不进行处理
                            pretty_log << "Duplicate data frame, ignored";
                        }
                    }

                    pretty_log << ::std::format("Send ack frame {}({})", seq_num, actual_forward_block_num);
                } else {
                    // 非当前窗口的重复的数据帧，不进行处理
                    pretty_log << ::std::format("Receive duplicate data frame {}({})", seq_num, actual_backward_block_num);
                    pretty_log << ::std::format("Send ack frame {}({})", seq_num, actual_backward_block_num);
                }

                // 发送/重发确认帧
                this->sendAckToPeer(seq_num);
            }

            // 对于既不在当前窗口也不在上一个窗口的数据帧，丢弃
        }
    }
} // namespace my

#endif // _SR_PROTOCOL_HPP_