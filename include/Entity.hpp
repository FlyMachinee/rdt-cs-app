

#include "pretty_log.hpp"
#include <format>
#include <winsock2.h>

namespace my
{

    class Peer
    {
    public:
        Peer() = default;
        Peer(sockaddr_in address) : m_address(address) {}
        Peer(sockaddr_in *pAddress) : m_address(*pAddress) {}
        virtual ~Peer() = default;

        sockaddr_in getAddr() const noexcept { return m_address; }
        const sockaddr *getAddrPtr() const { return reinterpret_cast<const sockaddr *>(&m_address); }
        const char *getIP() const noexcept { return inet_ntoa(m_address.sin_addr); }
        unsigned short getPort() const noexcept { return ntohs(m_address.sin_port); }

    protected:
        sockaddr_in m_address;
    };

    class Host : public Peer
    {
    public:
        Host() : m_socket(INVALID_SOCKET) {}
        virtual ~Host() = default;

        SOCKET getSocket() const noexcept { return m_socket; }
        void updateAddr()
        {
            if (m_socket == INVALID_SOCKET) {
                pretty_out << "throw from Host::updateAddr(): m_socket is INVALID_SOCKET";
                throw std::runtime_error("m_socket is INVALID_SOCKET");
            }
            int len = sizeof(m_address);
            if (getsockname(m_socket, reinterpret_cast<sockaddr *>(&m_address), &len) == SOCKET_ERROR) {
                pretty_out << ::std::format("throw from Host::updateAddr(): getsockname() failed, WSAGetLastError() = {0}", WSAGetLastError());
                throw std::runtime_error("getsockname() failed");
            }
        }

    protected:
        SOCKET m_socket;
    };
} // namespace my
