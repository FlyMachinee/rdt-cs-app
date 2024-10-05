#include "../include/GBN_Protocol.hpp"
#include "../include/SR_Protocol.hpp"
#include "../include/StopWait_Protocol.hpp"
#include "../include/wsa_wapper.h"

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

int main(int argc, char const *argv[])
{
    ::my::init_wsa();

    SOCKET host_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (host_socket == INVALID_SOCKET) {
        ::my::pretty_err << ::std::format("Create socket failed. Error code: {}", WSAGetLastError());
        return -1;
    }

    SOCKADDR_IN host_addr;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(12345);
    host_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(host_socket, reinterpret_cast<SOCKADDR *>(&host_addr), sizeof(host_addr)) == SOCKET_ERROR) {
        ::my::pretty_err << ::std::format("Bind socket failed. Error code: {}", WSAGetLastError());
        return -1;
    }

    // https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(host_socket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, nullptr, 0, &dwBytesReturned, nullptr, nullptr);

    ::my::SR_Transceiver<5, 10> transceiver(host_socket);
    transceiver.setPeer("127.0.0.1", 54321);
    transceiver.sendtoPeer("../server_repo/Little Prince.txt");

    ::my::cleanup_wsa();
    return 0;
}

#undef SIO_UDP_CONNRESET