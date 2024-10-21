#include <cstring>
#include <format>

#include "../include/UDPDataframe.h"
#include "../include/pretty_log.hpp"

/**
 * @brief 构造一个空的UDP数据帧
 */
my::UDPDataframe::UDPDataframe()
{
    m_data = new char[MAX_SIZE + 1];
    m_data[0] = NONE;
    m_size = 0;
}

/**
 * @brief 从接收到的数据构造一个UDP数据帧
 * @note buffer中的内容需要符合UDP数据帧的格式，即DATA、ACK或CMD之一
 *
 * @param buffer 接收到的数据
 * @param recv_size 接收到的数据的大小
 */
my::UDPDataframe::UDPDataframe(const char *buffer, int recv_size) : m_size(recv_size)
{
    m_data = new char[MAX_SIZE + 1];
    if (buffer[0] != ACK && buffer[0] != DATA && buffer[0] != CMD) {
        pretty_out << ::std::format("throw from UDPDataframe::UDPDataframe(): Invalid UDPDataframe type, buffer[0] = {0}", (int)buffer[0]);
        throw std::runtime_error("Invalid UDPDataframe type");
    }
    if (recv_size > MAX_DATA_SIZE + 4) {
        pretty_out << ::std::format("throw from UDPDataframe::UDPDataframe(): Size too large, recv_size = {0}", recv_size);
        throw std::runtime_error("Size too large");
    }
    ::std::memcpy(m_data, buffer, recv_size);
}

/**
 * @brief 拷贝构造函数
 *
 * @param other 另一个UDP数据帧
 */
my::UDPDataframe::UDPDataframe(const UDPDataframe &other) : m_size(other.m_size)
{
    m_data = new char[MAX_SIZE + 1];
    ::std::memcpy(m_data, other.m_data, m_size);
}

/**
 * @brief 移动构造函数
 *
 * @param other 另一个UDP数据帧
 */
my::UDPDataframe::UDPDataframe(UDPDataframe &&other) noexcept
    : m_data(other.m_data), m_size(other.m_size)
{
    // 置空原对象
    other.m_data = nullptr;
    other.m_size = 0;
}

/**
 * @brief 拷贝赋值运算符
 *
 * @param other 另一个UDP数据帧
 */
my::UDPDataframe &my::UDPDataframe::operator=(const UDPDataframe &other)
{
    if (this != &other) {
        // 进行深拷贝
        m_size = other.m_size;
        ::std::memcpy(m_data, other.m_data, m_size);
    }
    return *this;
}

/**
 * @brief 移动赋值运算符
 *
 * @param other 另一个UDP数据帧
 */
my::UDPDataframe &my::UDPDataframe::operator=(UDPDataframe &&other) noexcept
{
    if (this != &other) {
        // 释放原有资源
        delete[] m_data;

        // 移动资源
        m_data = other.m_data;
        m_size = other.m_size;

        // 置空原对象
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

my::UDPDataframe::~UDPDataframe()
{
    delete[] m_data;
}

/**
 * @brief 设置UDP数据帧的类型
 * @param type UDP数据帧的类型
 */
void my::UDPDataframe::setType(Type type)
{
    m_data[0] = (unsigned char)type;
}

/**
 * @brief 获取UDP数据帧的类型
 * @return UDP数据帧的类型
 */
my::UDPDataframe::Type my::UDPDataframe::getType() const
{
    return (Type)m_data[0];
}

/**
 * @brief 判断UDP数据帧是否有效
 * @note 即是否是DATA、ACK或CMD之一
 * @return 是否有效
 */
bool my::UDPDataframe::isValid() const noexcept
{
    return m_data[0] == ACK || m_data[0] == DATA || m_data[0] == CMD;
}

/**
 * @brief 判断UDP数据帧是否是ACK分组
 * @return 是否是ACK
 */
bool my::UDPDataframe::isAck() const noexcept
{
    return m_data[0] == ACK;
}

/**
 * @brief 判断UDP数据帧是否是ACK分组，并且ACK号为ack_num
 * @param ack_num ACK号
 * @return 是否是ACK并且ACK号为ack_num
 */
bool my::UDPDataframe::isAck(unsigned char ack_num) const noexcept
{
    return m_data[0] == ACK && m_data[1] == ack_num;
}

/**
 * @brief 判断UDP数据帧是否是DATA分组
 * @return 是否是DATA
 */
bool my::UDPDataframe::isData() const noexcept
{
    return m_data[0] == DATA;
}

/**
 * @brief 判断UDP数据帧是否是CMD分组
 * @return 是否是CMD
 */
bool my::UDPDataframe::isCmd() const noexcept
{
    return m_data[0] == CMD;
}

/**
 * @brief 获取UDP数据分组中的数据
 * @param &data_size 用于接收数据的大小
 * @return 数据的指针
 */
const char *my::UDPDataframe::data(int &data_size) const
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::data(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    data_size = *reinterpret_cast<const short *>(m_data + 2);
    return m_data + 4;
}

/**
 * @brief 获取UDP命令分组中的命令字符串
 * @return C风格命令字符串
 */
const char *my::UDPDataframe::cmd() const
{
    if (!isCmd()) {
        pretty_out << "throw from UDPDataframe::cmd(): Not a CMD frame";
        throw std::runtime_error("Not a CMD frame");
    }
    return m_data + 1;
}

/**
 * @brief 获取UDP数据帧的数据分组编号
 * @return 数据分组编号
 */
unsigned char my::UDPDataframe::getDataNum() const
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::dataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    return *reinterpret_cast<const unsigned char *>(m_data + 1);
}

/**
 * @brief 设置UDP数据分组的数据分组编号
 * @param data_num 数据分组编号
 */
void my::UDPDataframe::setDataNum(unsigned char data_num)
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::transDataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    m_data[1] = data_num;
}

/**
 * @brief 获取UDP确认分组的ACK分组编号
 * @return ACK分组编号
 */
unsigned char my::UDPDataframe::getAckNum() const
{
    if (!isAck()) {
        pretty_out << "throw from UDPDataframe::ackNum(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    return *reinterpret_cast<const unsigned char *>(m_data + 1);
}

/**
 * @brief 设置UDP确认分组的ACK分组编号
 * @param ack_num ACK分组编号
 */
void my::UDPDataframe::setAckNum(unsigned char ack_num)
{
    if (!isAck()) {
        pretty_out << "throw from UDPDataframe::transAckNum(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    m_data[1] = ack_num;
}

/**
 * @brief 构造一个ACK分组
 * @param ack_num ACK分组编号
 * @return ACK分组
 */
my::UDPDataframe my::UDPAck(unsigned char ack_num)
{
    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::ACK;
    frame.m_data[1] = ack_num;
    frame.m_size = 2;
    return frame;
}

/**
 * @brief 构造一个DATA分组
 * @param data_num 数据分组编号
 * @param data 数据
 * @param data_size 数据大小
 * @return DATA分组
 */
my::UDPDataframe my::UDPData(unsigned char data_num, const char *data, short data_size)
{
    if (data_size > UDPDataframe::MAX_DATA_SIZE) {
        pretty_out << ::std::format("throw from my::UDPData(): Size too large, data_size = {0}, MAX_DATA_SIZE = {1}", data_size, UDPDataframe::MAX_DATA_SIZE);
        throw std::runtime_error("Size too large");
    }

    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::DATA;
    frame.m_data[1] = data_num;
    *reinterpret_cast<short *>(frame.m_data + 2) = (short)data_size;
    ::std::memcpy(frame.m_data + 4, data, data_size);
    frame.m_size = data_size + 4;
    return frame;
}

/**
 * @brief 构造一个CMD分组
 * @param cmd 命令字符串
 * @return CMD分组
 */
my::UDPDataframe my::UDPCmd(::std::string_view cmd)
{
    if (cmd.size() > UDPDataframe::MAX_DATA_SIZE) {
        pretty_out << ::std::format("throw from my::UDPCmd(): Size too large, cmd.size() = {0}, MAX_DATA_SIZE = {1}", cmd.size(), UDPDataframe::MAX_DATA_SIZE);
        throw std::runtime_error("Size too large");
    }

    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::CMD;
    ::std::memcpy(frame.m_data + 1, cmd.data(), cmd.size());
    frame.m_size = cmd.size() + 1;
    return frame;
}

/**
 * @brief 在指定主机接收某一个由对方主机发来的一个UDP数据帧
 * @param host 主机
 * @param peer_from 用于接收对方主机信息
 * @return UDP数据帧
 */
my::UDPDataframe my::recvUDPDataframeFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame;

    sockaddr_in peer_addr;
    int addr_len = sizeof(peer_addr);
    int recv_size = recvfrom(host.getSocket(), frame.m_data, frame.MAX_SIZE, 0, reinterpret_cast<sockaddr *>(&peer_addr), &addr_len);

    if (recv_size == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::recvUDPDataframeFrom(): recvfrom() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("recvfrom() failed");
    }
    frame.m_size = recv_size;
    peer_from = Peer(peer_addr);

    return frame;
}

/**
 * @brief 在指定主机发送一个UDP数据帧到指定的对方主机
 * @param dataframe UDP数据帧
 * @param host 主机
 * @param peer_to 对方主机
 */
void my::sendUDPDataframeTo(const UDPDataframe &dataframe, const Host &host, const Peer &peer_to)
{
    if (!dataframe.isValid()) {
        pretty_out << "throw from my::sendUDPDataframeTo(): Not a valid UDPDataframe";
        throw std::runtime_error("Not a valid UDPDataframe");
    }

    if (sendto(host.getSocket(), dataframe.m_data, dataframe.m_size, 0, peer_to.getAddrPtr(), sizeof(sockaddr)) == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::sendUDPDataframeTo(): sendto() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("sendto() failed");
    }
}

/**
 * @brief 在指定主机接收一个ACK分组
 * @param host 主机
 * @param peer_from 用于接收对方主机信息
 * @return ACK分组的ACK号
 */
unsigned char my::recvAckFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame = recvUDPDataframeFrom(host, peer_from);
    if (!frame.isAck()) {
        pretty_out << "throw from my::recvAckFrom(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    return frame.getAckNum();
}

/**
 * @brief 在指定主机发送一个ACK分组到指定的对方主机
 * @param ack_num ACK号
 * @param host 主机
 * @param peer_to 对方主机
 */
void my::sendAckTo(unsigned char ack_num, const Host &host, const Peer &peer_to)
{
    char buffer[3] = {UDPDataframe::ACK, (char)ack_num, 0};
    if (sendto(host.getSocket(), buffer, 2, 0, peer_to.getAddrPtr(), sizeof(sockaddr)) == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::sendAckTo(): sendto() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("sendto() failed");
    }
}

/**
 * @brief 在指定主机接收一个CMD分组
 * @param host 主机
 * @param peer_from 用于接收对方主机信息
 * @return CMD分组的命令字符串
 */
::std::string my::recvCmdFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame = recvUDPDataframeFrom(host, peer_from);
    if (!frame.isCmd()) {
        pretty_out << "throw from my::recvCmdFrom(): Not a CMD frame";
        throw std::runtime_error("Not a CMD frame");
    }
    return ::std::string(frame.cmd());
}

/**
 * @brief 在指定主机发送一个CMD分组到指定的对方主机
 * @param cmd 命令字符串
 * @param host 主机
 * @param peer_to 对方主机
 */
void my::sendCmdTo(::std::string_view cmd, const Host &host, const Peer &peer_to)
{
    char buffer[UDPDataframe::MAX_SIZE + 2];
    buffer[0] = UDPDataframe::CMD;
    ::std::memcpy(buffer + 1, cmd.data(), cmd.size());
    buffer[cmd.size() + 1] = '\0';

    if (sendto(host.getSocket(), buffer, cmd.size() + 2, 0, peer_to.getAddrPtr(), sizeof(sockaddr)) == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::sendCmdTo(): sendto() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("sendto() failed");
    }
}