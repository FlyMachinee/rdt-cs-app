#ifndef _RDT_CLIENT_HPP_
#define _RDT_CLIENT_HPP_

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <vector>

#include "./GBN_Protocol.hpp"
#include "./SR_Protocol.hpp"
#include "./StopWait_Protocol.hpp"
#include "./wsa_wapper.h"

namespace my
{
    // 要求：Transceiver有sendUDPDataframeToPeer、recvUDPDataframeFromPeer、sendAckToPeer、recvAckFromPeer
    // 并且多继承自BasicRole
    template <class Transceiver>
    class RDT_Client : protected Transceiver
    {
    public:
        RDT_Client();
        virtual ~RDT_Client();

        void run();

    protected:
        void sendCmdToPeer(::std::string_view cmd);
        void disableLoss();
        void enableLoss();

    private:
        ::std::string m_prompt = ">>> ";
        ::std::filesystem::path m_repo = "../client_repo/";

        int handle_user_input();
        int exec_cmd(::std::string_view cmd);
        void help();
        void handle_upload();
        void handle_download();
    };

    template <int seqNumBound>
    using StopWait_Client = RDT_Client<StopWait_Transceiver<seqNumBound>>;

    template <int senderWindowSize, int seqNumBound>
    using GBN_Client = RDT_Client<GBN_Transceiver<senderWindowSize, seqNumBound>>;

    template <int windowSize, int seqNumBound>
    using SR_Client = RDT_Client<SR_Transceiver<windowSize, seqNumBound>>;

    template <class Transceiver>
    RDT_Client<Transceiver>::RDT_Client()
    {
        if (!::my::wsa_initialized) {
            ::my::init_wsa();
        }

        SOCKET host_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (host_socket == INVALID_SOCKET) {
            ::my::pretty_err << ::std::format("Create socket failed. Error code: {}", WSAGetLastError());
            throw ::std::runtime_error("Create socket failed");
        }

        SOCKADDR_IN host_addr;
        host_addr.sin_family = AF_INET;
        host_addr.sin_port = htons(0); // 0代表系统自动分配端口
        host_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (bind(host_socket, reinterpret_cast<SOCKADDR *>(&host_addr), sizeof(host_addr)) == SOCKET_ERROR) {
            ::my::pretty_err << ::std::format("Bind socket failed. Error code: {}", WSAGetLastError());
            throw ::std::runtime_error("Bind socket failed");
        }

        this->m_host.setSocket(host_socket);
        this->m_host.updateAddr();

        this->setPeer("127.0.0.1", 12345);

        // https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(host_socket, _WSAIOW(IOC_VENDOR, 12), &bNewBehavior, sizeof bNewBehavior, nullptr, 0, &dwBytesReturned, nullptr, nullptr);

        pretty_log << "Client initialized" << ::std::format("Running on {}", this->m_host.toString());
    }

    template <class Transceiver>
    RDT_Client<Transceiver>::~RDT_Client()
    {
        if (closesocket(this->m_host.getSocket()) == SOCKET_ERROR) {
            ::my::pretty_err << ::std::format("Close socket failed. Error code: {}", WSAGetLastError());
        }
        if (::my::wsa_initialized) {
            ::my::cleanup_wsa();
        }

        pretty_log << "Client closed";
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::run()
    {
        while (true) {
            this->disableLoss();
            try {
                if (this->handle_user_input() != 0) {
                    break;
                }
            } catch (const std::exception &e) {
                pretty_err << ::std::format("catch by RDT_Client::run():") << e.what();
            }
        }
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::sendCmdToPeer(::std::string_view cmd)
    {
        sendCmdTo(cmd, this->m_host, this->m_peer);
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::disableLoss()
    {
        this->disableReceiverLoss();
        this->disableSenderLoss();
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::enableLoss()
    {
        this->enableReceiverLoss();
        this->enableSenderLoss();
    }

    template <class Transceiver>
    int RDT_Client<Transceiver>::handle_user_input()
    {
        ::std::cout << m_prompt;
        ::std::string cmd;
        ::std::getline(::std::cin, cmd);
        return this->exec_cmd(cmd);
    }

    template <class Transceiver>
    int RDT_Client<Transceiver>::exec_cmd(::std::string_view cmd)
    {
        ::std::istringstream iss(cmd.data());
        ::std::string token;
        iss >> token;

        if (token == "upload" || token == "download") {
            bool is_upload = token == "upload";

            while (iss >> token) {
                if (token == "-ip") {
                    iss >> token;
                    try {
                        this->setPeerIp(token);
                    } catch (const std::exception &e) {
                        pretty_err << ::std::format("Invalid ip: \"{}\"", token);
                        return 0;
                    }
                } else if (token == "-port") {
                    iss >> token;
                    this->setPeerPort(::std::stoi(token));
                } else {
                    pretty_err << ::std::format("Unknown option \"{}\". Use \"help\" to get help", token);
                    return 0;
                }
            }

            if (is_upload) {
                this->handle_upload();
            } else {
                this->handle_download();
            }
        } else if (token == "repo") {
            bool is_set = false;
            while (iss >> token) {
                if (token == "-set") {
                    iss >> token;
                    if (!::std::filesystem::exists(token) || !::std::filesystem::is_directory(token)) {
                        pretty_err << ::std::format("Invalid directory: \"{}\"", token);
                        return 0;
                    }
                    is_set = true;
                    m_repo = token;
                } else {
                    pretty_err << ::std::format("Unknown option \"{}\". Use \"help\" to get help", token);
                    return 0;
                }
            }
            if (!is_set) {
                pretty_log << ::std::format("Client repository: \"{}\"", m_repo.string());
            } else {
                pretty_log << ::std::format("Client repository set to: \"{}\"", m_repo.string());
            }
        } else if (token == "help") {
            help();
        } else if (token == "exit" || token == "quit") {
            return -1;
        } else {
            pretty_err << "Unknown command. Use \"help\" to get help";
        }

        return 0;
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::help()
    {
        pretty_log
            << "Commands:"
            << "  upload [-ip <ip>] [-port <port>] - Upload file to server"
            << "    Default ip:port is 127.0.0.1:12345"
            << "  download [-ip <ip>] [-port <port>] - Download file from server"
            << "    Default ip:port is 127.0.0.1:12345"
            << "  repo [-set <dir_path>] - Show or set client repository"
            << "  help - Show help message"
            << "  exit/quit - Exit client";
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::handle_upload()
    {
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::handle_download()
    {
        // 先从服务器获取文件信息列表
        // 然后选择要下载的文件
        // 选择文件后，发送下载请求

        // 获取文件列表
        pretty_log << ::std::format("Fetching file list from server {}...", this->m_peer.toString());
        this->sendCmdToPeer("ls");

        int cnt = 0;
        while (this->recvAckFromPeer() == -1) {
            if (++cnt > 20) {
                pretty_err << "Failed to fetch file list from server, timeout";
                return;
            }
        }

        // 读取文件列表
        ::std::vector<::std::string> file_list;
        ::std::vector<::std::string> file_size_list;
        int max_file_name_length = 8;
        while (true) {
            UDPDataframe dataframe = this->recvUDPDataframeFromPeer();
            int length;
            const char *data = dataframe.data(length);
            if (length == 0) {
                break;
            }
            ::std::string data_str(data, length);
            int pos = data_str.find_last_of(' ');
            ::std::string file_name = data_str.substr(0, pos);
            ::std::string file_size = data_str.substr(pos + 1);

            file_list.push_back(file_name);
            max_file_name_length = ::std::max(max_file_name_length, (int)file_name.size());
            file_size_list.push_back(file_size);
        }

        // 显示文件列表
        if (file_list.empty()) {
            pretty_log << "No file in server";
            return;
        }

        ::std::cout << "No  Filename" << ::std::string(max_file_name_length - 8, ' ') << "  Size" << ::std::endl;
        for (int i = 0; i < file_list.size(); ++i) {
            ::std::cout << ::std::vformat(::std::format("{{:<4}}{{:<{}}}", max_file_name_length), ::std::make_format_args(i, file_list[i])) << "  " << file_size_list[i] << ::std::endl;
        }

        // 选择文件
        pretty_log << "Choose a file to download (input the number):";
        int file_num;

        while (true) {
            ::std::cout << ">>> ";
            ::std::string file_num_str;
            ::std::getline(::std::cin, file_num_str);
            if (::std::all_of(file_num_str.begin(), file_num_str.end(), ::isdigit)) {
                file_num = ::std::stoi(file_num_str);
                if (file_num >= 0 && file_num < file_list.size()) {
                    break;
                } else {
                    pretty_err << "Number out of range, please input again";
                    continue;
                }
            } else {
                pretty_err << "Invalid input, please input a number";
                continue;
            }
        }

        ::std::string file_fullname = file_list[file_num];
        int dot_pos = file_fullname.find_last_of('.');
        ::std::string file_ext = file_fullname.substr(dot_pos);
        ::std::string file_name = file_fullname.substr(0, dot_pos);

        ::std::filesystem::path file_path;
        for (int i = 0;; ++i) {
            file_path = m_repo / (i == 0 ? file_fullname : ::std::format("{}({}){}", file_fullname, i, file_ext));
            if (!::std::filesystem::exists(file_path)) {
                break;
            }
        }
        pretty_log << ::std::format("The file will be saved to: \"{}\"", file_path.string());

        // 发送下载请求
        this->sendCmdToPeer(::std::format("download {}", file_fullname));
        cnt = 0;
        while (this->recvAckFromPeer() == -1) {
            if (++cnt > 20) {
                pretty_err << "Failed to download file, timeout";
                return;
            }
        }

        // 接收文件
        enableLoss();
        this->recvfromPeer(file_path.string());
        disableLoss();

        pretty_log
            << ::std::format("Download file \"{}\" successfully from {}", file_fullname, this->m_peer.toString())
            << ::std::format("Saved to: \"{}\"", file_path.string());
    }

} // namespace my

#endif // _RDT_CLIENT_HPP_