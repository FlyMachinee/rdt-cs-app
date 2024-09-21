
#include "./Entity.hpp"

namespace my
{
    class BasicRole
    {
    public:
        BasicRole() = default;
        virtual ~BasicRole() = 0;
        BasicRole(const BasicRole &) = delete;
        BasicRole &operator=(const BasicRole &) = delete;

    protected:
        const int m_windowWidth; // W

        const int m_serialDomain; // 2**k

        Host m_host;
        Peer m_peer;
    };
} // namespace my
