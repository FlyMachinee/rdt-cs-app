#ifndef _STOPWAIT_PROTOCOL_HPP_
#define _STOPWAIT_PROTOCOL_HPP_

#include "./GBN_Protocol.hpp"

namespace my
{
    template <int seqNumBound>
    using StopWait_Sender = GBN_Sender<1, seqNumBound>;

    template <int seqNumBound>
    using StopWait_Receiver = GBN_Receiver<seqNumBound>;

    template <int seqNumBound>
    using StopWait_Transceiver = GBN_Transceiver<1, seqNumBound>;
} // namespace my

#endif // _STOPWAIT_PROTOCOL_HPP_