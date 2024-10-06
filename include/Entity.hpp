#ifndef _ENTITY_HPP_
#define _ENTITY_HPP_

#include "./pretty_log.hpp"
#include <format>
#include <winsock2.h>

namespace my
{
    class BasicRole;

    class Peer
    {
    public:
        friend class BasicRole;

        Peer() = default;
        Peer(sockaddr_in address) : m_address(address) {}
        Peer(sockaddr_in *pAddress) : m_address(*pAddress) {}
        virtual ~Peer() = default;

        void setAddr(sockaddr_in address) noexcept { m_address = address; }
        sockaddr_in getAddr() const noexcept { return m_address; }
        const sockaddr *getAddrPtr() const { return reinterpret_cast<const sockaddr *>(&m_address); }

        const char *getIP() const noexcept { return inet_ntoa(m_address.sin_addr); }
        unsigned short getPort() const noexcept { return ntohs(m_address.sin_port); }

        bool operator==(const Peer &peer) const noexcept
        {
            return m_address.sin_addr.s_addr == peer.m_address.sin_addr.s_addr &&
                   m_address.sin_port == peer.m_address.sin_port;
        }

        ::std::string toString() const
        {
            return ::std::format("{0}:{1}", getIP(), getPort());
        }

    protected:
        sockaddr_in m_address;
    };

    class Host : public Peer
    {
    public:
        Host() : m_socket(INVALID_SOCKET) {}
        Host(SOCKET host_socket) : m_socket(host_socket)
        {
            if (host_socket != INVALID_SOCKET) updateAddr();
        }
        Host(SOCKET host_socket, sockaddr_in address) : Peer(address), m_socket(host_socket) {}
        virtual ~Host() = default;

        SOCKET getSocket() const noexcept { return m_socket; }
        void setSocket(SOCKET host_socket) noexcept { m_socket = host_socket; }

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

#endif // _ENTITY_HPP_