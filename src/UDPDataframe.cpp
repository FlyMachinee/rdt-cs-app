#include <cstring>
#include <format>

#include "../include/UDPDataframe.h"
#include "../include/pretty_log.hpp"

my::UDPDataframe::UDPDataframe()
{
    m_data = new char[MAX_SIZE + 1];
    m_data[0] = NONE;
    m_size = 0;
}

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

my::UDPDataframe::UDPDataframe(const UDPDataframe &other) : m_size(other.m_size)
{
    m_data = new char[MAX_SIZE + 1];
    ::std::memcpy(m_data, other.m_data, m_size);
}

my::UDPDataframe::UDPDataframe(UDPDataframe &&other) noexcept
    : m_data(other.m_data), m_size(other.m_size)
{
    other.m_data = nullptr;
    other.m_size = 0;
}

my::UDPDataframe &my::UDPDataframe::operator=(const UDPDataframe &other)
{
    if (this != &other) {
        m_size = other.m_size;
        ::std::memcpy(m_data, other.m_data, m_size);
    }
    return *this;
}

my::UDPDataframe &my::UDPDataframe::operator=(UDPDataframe &&other) noexcept
{
    if (this != &other) {
        delete[] m_data;
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

my::UDPDataframe::~UDPDataframe()
{
    delete[] m_data;
}

void my::UDPDataframe::setType(Type type)
{
    m_data[0] = (char)type;
}

bool my::UDPDataframe::isValid() const noexcept
{
    return m_data[0] == ACK || m_data[0] == DATA || m_data[0] == CMD;
}

bool my::UDPDataframe::isAck() const noexcept
{
    return m_data[0] == ACK;
}

bool my::UDPDataframe::isAck(char ack_num) const noexcept
{
    return m_data[0] == ACK && m_data[1] == ack_num;
}

bool my::UDPDataframe::isData() const noexcept
{
    return m_data[0] == DATA;
}

bool my::UDPDataframe::isCmd() const noexcept
{
    return m_data[0] == CMD;
}

const char *my::UDPDataframe::data(int &data_size) const
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::data(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    data_size = *reinterpret_cast<const short *>(m_data + 2);
    return m_data + 4;
}

const char *my::UDPDataframe::cmd() const
{
    if (!isCmd()) {
        pretty_out << "throw from UDPDataframe::cmd(): Not a CMD frame";
        throw std::runtime_error("Not a CMD frame");
    }
    return m_data + 1;
}

char my::UDPDataframe::getDataNum() const
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::dataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    return *reinterpret_cast<const char *>(m_data + 1);
}

void my::UDPDataframe::setDataNum(char data_num)
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::transDataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    m_data[1] = (char)data_num;
}

char my::UDPDataframe::getAckNum() const
{
    if (!isAck()) {
        pretty_out << "throw from UDPDataframe::ackNum(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    return *reinterpret_cast<const char *>(m_data + 1);
}

void my::UDPDataframe::setAckNum(char ack_num)
{
    if (!isAck()) {
        pretty_out << "throw from UDPDataframe::transAckNum(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    m_data[1] = (char)ack_num;
}

my::UDPDataframe my::UDPAck(char ack_num)
{
    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::ACK;
    frame.m_data[1] = (char)ack_num;
    frame.m_size = 2;
    return frame;
}

my::UDPDataframe my::UDPData(char data_num, const char *data, int data_size)
{
    if (data_size > UDPDataframe::MAX_DATA_SIZE) {
        pretty_out << ::std::format("throw from my::UDPData(): Size too large, data_size = {0}, MAX_DATA_SIZE = {1}", data_size, UDPDataframe::MAX_DATA_SIZE);
        throw std::runtime_error("Size too large");
    }

    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::DATA;
    frame.m_data[1] = (char)data_num;
    *reinterpret_cast<short *>(frame.m_data + 2) = (short)data_size;
    ::std::memcpy(frame.m_data + 4, data, data_size);
    frame.m_size = data_size + 4;
    return frame;
}

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

char my::recvAckFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame = recvUDPDataframeFrom(host, peer_from);
    if (!frame.isAck()) {
        pretty_out << "throw from my::recvAckFrom(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    return frame.getAckNum();
}

void my::sendAckTo(char ack_num, const Host &host, const Peer &peer_to)
{
    char buffer[3] = {UDPDataframe::ACK, ack_num, 0};
    if (sendto(host.getSocket(), buffer, 2, 0, peer_to.getAddrPtr(), sizeof(sockaddr)) == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::sendAckTo(): sendto() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("sendto() failed");
    }
}

::std::string my::recvCmdFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame = recvUDPDataframeFrom(host, peer_from);
    if (!frame.isCmd()) {
        pretty_out << "throw from my::recvCmdFrom(): Not a CMD frame";
        throw std::runtime_error("Not a CMD frame");
    }
    return ::std::string(frame.cmd());
}

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