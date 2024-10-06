#ifndef _RDT_SERVER_HPP_
#define _RDT_SERVER_HPP_

#include <filesystem>
#include <sstream>

#include "./GBN_Protocol.hpp"
#include "./SR_Protocol.hpp"
#include "./StopWait_Protocol.hpp"
#include "./wsa_wapper.h"

namespace my
{
    // 要求：Transceiver有sendUDPDataframeToPeer、recvUDPDataframeFromPeer、sendAckToPeer、recvAckFromPeer
    // 并且多继承自BasicRole
    template <class Transceiver>
    class RDT_Server : protected Transceiver
    {
    public:
        RDT_Server();
        virtual ~RDT_Server();

        void run();

    protected:
        ::std::string recvCmdFromPeer();
        void disableLoss();
        void enableLoss();

    private:
        ::std::string m_prompt = ">>> ";
        ::std::filesystem::path m_repo = "../server_repo/";

        int exec_cmd(::std::string_view cmd);
        void handle_ls();
        void handle_download(::std::string_view filename);
        void handle_upload(::std::string_view filename);
    };

    template <int seqNumBound>
    using StopWait_Server = RDT_Server<StopWait_Transceiver<seqNumBound>>;

    template <int senderWindowSize, int seqNumBound>
    using GBN_Server = RDT_Server<GBN_Transceiver<senderWindowSize, seqNumBound>>;

    template <int windowSize, int seqNumBound>
    using SR_Server = RDT_Server<SR_Transceiver<windowSize, seqNumBound>>;

    template <class Transceiver>
    RDT_Server<Transceiver>::RDT_Server()
    {
        if (!wsa_initialized) {
            init_wsa();
        }

        SOCKET host_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (host_socket == INVALID_SOCKET) {
            ::my::pretty_err << ::std::format("Create socket failed. Error code: {}", WSAGetLastError());
            throw ::std::runtime_error("Create socket failed");
        }

        SOCKADDR_IN host_addr;
        host_addr.sin_family = AF_INET;
        host_addr.sin_port = htons(12345);
        host_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (bind(host_socket, reinterpret_cast<SOCKADDR *>(&host_addr), sizeof(host_addr)) == SOCKET_ERROR) {
            ::my::pretty_err << ::std::format("Bind socket failed. Error code: {}", WSAGetLastError());
            throw ::std::runtime_error("Bind socket failed");
        }

        this->m_host.setSocket(host_socket);
        this->m_host.updateAddr();

        // https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(host_socket, _WSAIOW(IOC_VENDOR, 12), &bNewBehavior, sizeof bNewBehavior, nullptr, 0, &dwBytesReturned, nullptr, nullptr);

        if (!::std::filesystem::exists(m_repo)) {
            ::std::filesystem::create_directory(m_repo);
        }

        pretty_log << "Server initialized" << ::std::format("Running on {}", this->m_host.toString());
    }

    template <class Transceiver>
    RDT_Server<Transceiver>::~RDT_Server()
    {
        if (closesocket(this->m_host.getSocket()) == SOCKET_ERROR) {
            ::my::pretty_err << ::std::format("Close socket failed. Error code: {}", WSAGetLastError());
            return;
        }
        pretty_log << "Server closed";

        if (wsa_initialized) {
            cleanup_wsa();
        }
    }

    template <class Transceiver>
    void RDT_Server<Transceiver>::run()
    {
        while (true) {
            this->disableLoss();
            try {
                ::std::string cmd = recvCmdFromPeer();
                pretty_out << ::std::format("[{}] {}{}", this->m_peer.toString(), m_prompt, cmd);
                exec_cmd(cmd);
            } catch (const ::std::runtime_error &e) {
                pretty_out << ::std::format("catch by RDT_Server::run():") << e.what();
            }
        }
    }

    template <class Transceiver>
    inline ::std::string RDT_Server<Transceiver>::recvCmdFromPeer()
    {
        // 在接收命令时确定客户端地址
        return recvCmdFrom(this->m_host, this->m_peer);
    }

    template <class Transceiver>
    inline void RDT_Server<Transceiver>::disableLoss()
    {
        this->disableReceiverLoss();
        this->disableSenderLoss();
    }

    template <class Transceiver>
    inline void RDT_Server<Transceiver>::enableLoss()
    {
        this->enableReceiverLoss();
        this->enableSenderLoss();
    }

    template <class Transceiver>
    inline int RDT_Server<Transceiver>::exec_cmd(::std::string_view cmd)
    {
        ::std::istringstream iss(cmd.data());
        ::std::string token;
        iss >> token;

        if (token == "ls") {
            this->sendAckToPeer(0);
            handle_ls();
        } else if (token == "download") {
            ::std::getline(iss, token);
            this->sendAckToPeer(0);
            handle_download(token.substr(1));
        } else if (token == "upload") {
            ::std::getline(iss, token);
            this->sendAckToPeer(0);
            handle_upload(token.substr(1));
        } else {
            pretty_err << ::std::format("Unknown command: \"{}\"", token);
        }

        return 0;
    }

    template <class Transceiver>
    inline void RDT_Server<Transceiver>::handle_ls()
    {
        for (const auto &entry : ::std::filesystem::directory_iterator(m_repo)) {
            if (entry.is_regular_file()) {
                ::std::string str =
                    entry.path().filename().string() +
                    " " +
                    ::std::to_string(entry.file_size());
                sendUDPDataframeTo(UDPData(0, str.c_str(), str.size()), this->m_host, this->m_peer);
            }
        }
        char buffer[1] = {0};
        sendUDPDataframeTo(UDPData(0, buffer, 0), this->m_host, this->m_peer);
    }

    template <class Transceiver>
    inline void RDT_Server<Transceiver>::handle_download(::std::string_view filename)
    {
        enableLoss();
        this->sendtoPeer(m_repo.string() + ::std::string(filename));
        disableLoss();
    }

    template <class Transceiver>
    inline void RDT_Server<Transceiver>::handle_upload(::std::string_view filename)
    {
        // 不考虑文件与文件夹同名的情况
        ::std::filesystem::path file_path = m_repo / ::std::string(filename);
        if (::std::filesystem::exists(file_path)) {
            pretty_log << ::std::format("File \"{}\" already exists, overwrite", file_path.string());
            ::std::filesystem::remove(file_path);
        }
        enableLoss();
        this->recvfromPeer(file_path.string());
        disableLoss();
    }

} // namespace my

#endif // _RDT_SERVER_HPP_