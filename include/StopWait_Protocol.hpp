#ifndef _STOPWAIT_PROTOCOL_HPP_
#define _STOPWAIT_PROTOCOL_HPP_

#include "./GBN_Protocol.hpp"

namespace my
{
    /**
     * @brief 停等协议的发送方
     * @tparam seqNumBound 序列号的上界
     */
    template <int seqNumBound>
    using StopWait_Sender = GBN_Sender<1, seqNumBound>;

    /**
     * @brief 停等协议的接收方
     * @tparam seqNumBound 序列号的上界
     */
    template <int seqNumBound>
    using StopWait_Receiver = GBN_Receiver<seqNumBound>;

    /**
     * @brief 停等协议发送方与接收方的组合体
     * @tparam seqNumBound 序列号的上界
     */
    template <int seqNumBound>
    using StopWait_Transceiver = GBN_Transceiver<1, seqNumBound>;
} // namespace my

#endif // _STOPWAIT_PROTOCOL_HPP_