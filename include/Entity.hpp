#ifndef _ENTITY_HPP_
#define _ENTITY_HPP_

#include "./pretty_log.hpp"
#include <format>
#include <winsock2.h>

namespace my
{
    class BasicRole;

    /**
     * @brief Peer类表示一个对等方，对sockaddr_in结构体的封装，语意上表示对方进程（对发送者来说就是接收者，反之亦然）
     */
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

        /**
         * @brief 获取对等方的字符串表示
         * @return 对等方的字符串表示
         */
        ::std::string toString() const
        {
            return ::std::format("{0}:{1}", getIP(), getPort());
        }

    protected:
        sockaddr_in m_address;
    };

    /**
     * @brief Host类表示一个主机，对Peer类的扩展，增加了一个SOCKET成员，语意上表示本进程
     */
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

        /**
         * @brief 通过现有的SOCKET更新IP地址和端口号信息
         */
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