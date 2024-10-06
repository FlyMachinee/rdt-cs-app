#ifndef _BASIC_ROLE_H_
#define _BASIC_ROLE_H_

#include <random>

#include "./Entity.hpp"

namespace my
{
    class BasicRole
    {
    public:
        BasicRole() = default;
        BasicRole(SOCKET host_socket) : m_host(host_socket) {}
        virtual ~BasicRole() = 0;
        BasicRole(const BasicRole &) = delete;
        BasicRole &operator=(const BasicRole &) = delete;

        virtual void setPeer(::std::string_view ip, unsigned short port) final
        {
            m_peer.m_address.sin_family = AF_INET;
            m_peer.m_address.sin_port = htons(port);
            m_peer.m_address.sin_addr.s_addr = inet_addr(ip.data());
            if (m_peer.m_address.sin_addr.s_addr == INADDR_NONE) {
                pretty_out << "throw from BasicRole::setPeer(): Invalid IP address";
                throw std::runtime_error("Invalid IP address");
            }
        }
        virtual void setPeerIp(::std::string_view ip) final
        {
            m_peer.m_address.sin_family = AF_INET;
            m_peer.m_address.sin_addr.s_addr = inet_addr(ip.data());
            if (m_peer.m_address.sin_addr.s_addr == INADDR_NONE) {
                pretty_out << "throw from BasicRole::setPeerIp(): Invalid IP address";
                throw std::runtime_error("Invalid IP address");
            }
        }
        virtual void setPeerPort(unsigned short port) final { m_peer.m_address.sin_port = htons(port); }
        virtual void setPeer(sockaddr_in peer_address) final { m_peer.m_address = peer_address; }
        virtual void setPeer(const Peer &peer) final { m_peer = peer; }
        virtual void setTimeout(int timeout) final { m_timeout = timeout; }

        virtual float random() final { return m_distribution(m_engine); }

    protected:
        Host m_host;
        Peer m_peer;
        int m_timeout = 2000;

    private:
        static std::random_device m_device;
        static std::mt19937 m_engine;
        static std::uniform_real_distribution<float> m_distribution;
    };

    inline int getActualForwardBlockNum(int base, char ack_num, int seqNumBound) noexcept
    {
        int base_mod_M = base % seqNumBound;
        return (base + ack_num - base_mod_M) + (base_mod_M <= ack_num ? 0 : seqNumBound);
    }

    inline int getActualBackwardBlockNum(int base, char ack_num, int seqNumBound) noexcept
    {
        int num = (base - 1) % seqNumBound;
        return (base - 1 - num + ack_num) - (ack_num <= num ? 0 : seqNumBound);
    }

} // namespace my

#endif // _BASIC_ROLE_H_