#ifndef _BASIC_ROLE_H_
#define _BASIC_ROLE_H_

#include <random>

#include "./Entity.hpp"

namespace my
{
    /**
     * @brief BasicRole是一个抽象类，它是所有协议发送方和接收方的基类
     * @details 封装了本进程信息与对方进程信息，以及随机数生成功能
     */
    class BasicRole
    {
    public:
        BasicRole() = default;
        BasicRole(SOCKET host_socket) : m_host(host_socket) {}
        virtual ~BasicRole() = 0;
        BasicRole(const BasicRole &) = delete;
        BasicRole &operator=(const BasicRole &) = delete;

        /**
         * @brief 设置对方主机的IP地址和端口号
         * @param ip 对方主机的IP地址
         * @param port 对方主机的端口号
         */
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

        /**
         * @brief 设置对方主机的IP地址
         * @param ip 对方主机的IP地址
         */
        virtual void setPeerIp(::std::string_view ip) final
        {
            m_peer.m_address.sin_family = AF_INET;
            m_peer.m_address.sin_addr.s_addr = inet_addr(ip.data());
            if (m_peer.m_address.sin_addr.s_addr == INADDR_NONE) {
                pretty_out << "throw from BasicRole::setPeerIp(): Invalid IP address";
                throw std::runtime_error("Invalid IP address");
            }
        }

        /**
         * @brief 设置对方主机的端口号
         * @param port 对方主机的端口号
         */
        virtual void setPeerPort(unsigned short port) final { m_peer.m_address.sin_port = htons(port); }

        /**
         * @brief 设置对方主机的地址，直接从sockaddr_in结构体获取
         * @param peer_address 对方主机的地址
         */
        virtual void setPeer(sockaddr_in peer_address) final { m_peer.m_address = peer_address; }

        /**
         * @brief 设置对方主机的地址，通过Peer对象获取
         * @param peer 对方主机
         */
        virtual void setPeer(const Peer &peer) final { m_peer = peer; }

        /**
         * @brief 设置所有操作的超时时间
         * @param timeout 超时时间，单位为毫秒
         */
        virtual void setTimeout(int timeout) final { m_timeout = timeout; }

        /**
         * @brief 获取一个随机数，服从在[0, 1)上的均匀分布
         * @return 该随机数
         */
        virtual float random() final { return m_distribution(m_engine); }

    protected:
        Host m_host;
        Peer m_peer;
        int m_timeout = 1000;

    private:
        static std::random_device m_device;
        static std::mt19937 m_engine;
        static std::uniform_real_distribution<float> m_distribution;
    };

    /**
     * @brief 获取在序号数为seqNumBound、窗口起始块编号为base的情况下，ack_num在当前窗口对应的实际序号
     * @param base 基础块编号
     * @param ack_num 确认号
     * @param seqNumBound 序列号的上界
     * @return ack_num在当前窗口对应的实际序号
     */
    inline int getActualForwardBlockNum(int base, char ack_num, int seqNumBound) noexcept
    {
        int base_mod_M = base % seqNumBound;
        return (base + ack_num - base_mod_M) + (base_mod_M <= ack_num ? 0 : seqNumBound);
    }

    /**
     * @brief 获取在序号数为seqNumBound、窗口起始块编号为base的情况下，ack_num在上一个窗口对应的实际序号
     * @param base 基础块编号
     * @param ack_num 确认号
     * @param seqNumBound 序列号的上界
     * @return ack_num在上一个窗口对应的实际序号
     */
    inline int getActualBackwardBlockNum(int base, char ack_num, int seqNumBound) noexcept
    {
        int num = (base - 1) % seqNumBound;
        return (base - 1 - num + ack_num) - (ack_num <= num ? 0 : seqNumBound);
    }

} // namespace my

#endif // _BASIC_ROLE_H_