#ifndef _WSA_WRAPPER_H_INCLUDED_
#define _WSA_WRAPPER_H_INCLUDED_

#include <winsock2.h>

namespace my
{
    extern bool wsa_initialized;
    bool init_wsa();
    bool cleanup_wsa();
} // namespace my

#endif // _WSA_WRAPPER_H_INCLUDED_