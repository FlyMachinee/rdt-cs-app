
#include <cstring>
#include <format>

#include "../include/UDPDataframe.h"
#include "../include/pretty_log.hpp"

my::UDPDataframe::UDPDataframe()
{
    m_data[0] = NONE;
    m_size = 0;
}

my::UDPDataframe::UDPDataframe(const char *buffer, int recv_size) : m_size(recv_size)
{
    if (buffer[0] != ACK && buffer[0] != DATA) {
        pretty_out << ::std::format("throw from UDPDataframe::UDPDataframe(): Invalid UDPDataframe type, buffer[0] = {0}", (int)buffer[0]);
        throw std::runtime_error("Invalid UDPDataframe type");
    }
    if (recv_size > MAX_DATA_SIZE + 4) {
        pretty_out << ::std::format("throw from UDPDataframe::UDPDataframe(): Size too large, recv_size = {0}", recv_size);
        throw std::runtime_error("Size too large");
    }
    ::std::memcpy(m_data, buffer, recv_size);
}

bool my::UDPDataframe::isAck() const noexcept
{
    return m_data[0] == ACK;
}

bool my::UDPDataframe::isData() const noexcept
{
    return m_data[0] == DATA;
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

int my::UDPDataframe::getDataNum() const
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::dataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    return *reinterpret_cast<const char *>(m_data + 1);
}

void my::UDPDataframe::setDataNum(int data_num)
{
    transDataNum([data_num](int num) -> int { return data_num; });
}

void my::UDPDataframe::modDataNum(int modulus)
{
    transDataNum([modulus](int num) -> int { return num % modulus; });
}

void my::UDPDataframe::transDataNum(::std::function<int(int)> f)
{
    if (!isData()) {
        pretty_out << "throw from UDPDataframe::transDataNum(): Not a DATA frame";
        throw std::runtime_error("Not a DATA frame");
    }
    m_data[1] = (char)f(m_data[1]);
}

int my::UDPDataframe::getAckNum() const
{
    if (!isAck()) {
        pretty_out << "throw from UDPDataframe::ackNum(): Not an ACK frame";
        throw std::runtime_error("Not an ACK frame");
    }
    return *reinterpret_cast<const char *>(m_data + 1);
}

my::UDPDataframe my::UDPAck(int ack_num)
{
    UDPDataframe frame;
    frame.m_data[0] = UDPDataframe::ACK;
    frame.m_data[1] = (char)ack_num;
    frame.m_size = 2;
    return frame;
}

my::UDPDataframe my::UDPData(int data_num, const char *data, int data_size)
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

my::UDPDataframe my::recvUDPDataframeFrom(const Host &host, Peer &peer_from)
{
    UDPDataframe frame;

    sockaddr_in peer_addr;
    int recv_size = recvfrom(host.getSocket(), frame.m_data, frame.MAX_SIZE, 0, reinterpret_cast<sockaddr *>(&peer_addr), nullptr);

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
    if (sendto(host.getSocket(), dataframe.m_data, dataframe.m_size, 0, peer_to.getAddrPtr(), sizeof(sockaddr)) == SOCKET_ERROR) {
        pretty_out << ::std::format("throw from my::sendUDPDataframeTo(): sendto() failed, WSAGetLastError() = {0}", WSAGetLastError());
        throw std::runtime_error("sendto() failed");
    }
}