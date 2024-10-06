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
        bool handle_ls(::std::vector<::std::string> &file_list, ::std::vector<::std::string> &file_size_list);
        bool handle_lss(::std::vector<::std::string> &file_list, ::std::vector<::std::string> &file_size_list);
        void handle_upload();
        void handle_download();

        int get_num_input();
        void show_file_list(const ::std::vector<::std::string> &file_list, const ::std::vector<::std::string> &file_size_list);
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
        if (!wsa_initialized) {
            init_wsa();
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

        if (!::std::filesystem::exists(m_repo)) {
            ::std::filesystem::create_directory(m_repo);
        }

        pretty_log << "Client initialized" << ::std::format("Running on {}", this->m_host.toString());
    }

    template <class Transceiver>
    RDT_Client<Transceiver>::~RDT_Client()
    {
        if (closesocket(this->m_host.getSocket()) == SOCKET_ERROR) {
            ::my::pretty_err << ::std::format("Close socket failed. Error code: {}", WSAGetLastError());
            return;
        }
        pretty_log << "Client closed";

        if (wsa_initialized) {
            cleanup_wsa();
        }
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

        // 这里忽略了路径中有空格的情况
        // 处理起来比较麻烦，暂时不考虑 (正确方式为在输入路径时加引号)

        if (token == "upload" || token == "download" || token == "lss") {
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

            if (token == "upload") {
                this->handle_upload();
            } else if (token == "download") {
                this->handle_download();
            } else if (token == "lss") {
                ::std::vector<::std::string> file_list;
                ::std::vector<::std::string> file_size_list;
                this->handle_lss(file_list, file_size_list);
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
        } else if (token == "ls") {
            ::std::vector<::std::string> file_list;
            ::std::vector<::std::string> file_size_list;
            handle_ls(file_list, file_size_list);
        } else if (token == "help") {
            help();
        } else if (token == "loss") {
            bool is_set = false;

            if (iss >> token) {
                if (token == "-set") {
                    while (iss >> token) {
                        float rate;
                        auto get_rate = [&iss, &rate]() {
                            iss >> rate;
                            if (rate < 0 || rate > 1) {
                                pretty_err << "Invalid loss rate, should be in [0, 1]";
                                return false;
                            }
                            return true;
                        };

                        auto set_loss_rate = [&](auto set_loss_func) {
                            if (!get_rate()) {
                                return false;
                            }
                            (this->*set_loss_func)(rate);
                            is_set = true;
                            return true;
                        };

                        if (token == "sa") {
                            if (!set_loss_rate(&RDT_Client::setSendAckLoss)) {
                                return 0;
                            }
                        } else if (token == "sd") {
                            if (!set_loss_rate(&RDT_Client::setSendLoss)) {
                                return 0;
                            }
                        } else if (token == "ra") {
                            if (!set_loss_rate(&RDT_Client::setRecvAckLoss)) {
                                return 0;
                            }
                        } else if (token == "rd") {
                            if (!set_loss_rate(&RDT_Client::setRecvLoss)) {
                                return 0;
                            }
                        } else {
                            pretty_err << ::std::format("Unknown option \"{}\". Use \"help\" to get help", token);
                            return 0;
                        }
                    }
                } else {
                    pretty_err << ::std::format("Unknown option \"{}\". Use \"help\" to get help", token);
                    return 0;
                }
            }

            pretty_log << (is_set ? "Loss rate set to:" : "Loss rate:")
                       << ::std::format("(sa) client_send_ack_loss    {:.2f}", this->getSendAckLoss())
                       << ::std::format("(sd) client_send_data_loss   {:.2f}", this->getSendLoss())
                       << ::std::format("(ra) client_recv_ack_loss    {:.2f}", this->getRecvAckLoss())
                       << ::std::format("(rd) client_recv_data_loss   {:.2f}", this->getRecvLoss());

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
            << "Commands:\n"
            << "  upload [-ip <ip>] [-port <port>] - Upload file to server, create or overwrite"
            << "    Default ip:port is 127.0.0.1:12345\n"
            << "  download [-ip <ip>] [-port <port>] - Download file from server"
            << "    Default ip:port is 127.0.0.1:12345\n"
            << "  lss [-ip <ip>] [-port <port>] - List files in server repository"
            << "    Default ip:port is 127.0.0.1:12345\n"
            << "  ls - List files in client repository\n"
            << "  repo [-set <dir_path>] - Show or set client repository\n"
            << "  loss [-set < <loss_name> <loss_rate> ...>] - Show or set loss rate"
            << "    <loss_name>: sa - send_ack, sd - send_data, ra - recv_ack, rd - recv_data"
            << "    <loss_rate>: float, in [0, 1]"
            << "    e.g. loss -set sa 0.1 rd 0.2"
            << "         will set client_send_ack_loss to 0.1, client_recv_data_loss to 0.2\n"
            << "  help - Show help message\n"
            << "  exit/quit - Exit client";
    }

    template <class Transceiver>
    inline bool RDT_Client<Transceiver>::handle_ls(::std::vector<::std::string> &file_list, ::std::vector<::std::string> &file_size_list)
    {
        for (const auto &entry : ::std::filesystem::directory_iterator(m_repo)) {
            if (entry.is_regular_file()) {
                file_list.push_back(entry.path().filename().string());
                file_size_list.push_back(::std::to_string(entry.file_size()));
            }
        }

        if (file_list.empty()) {
            pretty_log << "No file in client repository";
            return false;
        }

        show_file_list(file_list, file_size_list);
        return true;
    }

    template <class Transceiver>
    inline bool RDT_Client<Transceiver>::handle_lss(::std::vector<::std::string> &file_list, ::std::vector<::std::string> &file_size_list)
    {
        this->sendCmdToPeer("ls");

        int cnt = 0;
        while (this->recvAckFromPeer() == -1) {
            if (++cnt > 20) {
                pretty_err << "Failed to fetch file list from server, timeout";
                return false;
            }
        }

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

        if (file_list.empty()) {
            pretty_log << "No file in server";
            return false;
        }

        show_file_list(file_list, file_size_list);
        return true;
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::handle_upload()
    {
        // 从本地获取文件列表
        // 选择要上传的文件
        // 选择文件后，发送上传请求

        // 获取文件列表
        pretty_log << "Fetching local file list...";
        ::std::vector<::std::string> file_list;
        ::std::vector<::std::string> file_size_list;
        if (!handle_ls(file_list, file_size_list)) {
            return;
        }

        // 选择文件
        pretty_log << "Choose a file to upload (input the number):";
        int file_num;
        while (true) {
            file_num = get_num_input();
            if (file_num >= 0 && file_num < file_list.size()) {
                break;
            } else {
                pretty_err << "File number out of range, please input again";
            }
        }
        pretty_log << ::std::format("The file will be uploaded to server {}, create or overwrite", this->m_peer.toString());

        // 发送上传请求
        this->sendCmdToPeer(::std::format("upload {}", file_list[file_num]));
        int cnt = 0;
        while (this->recvAckFromPeer() == -1) {
            if (++cnt > 20) {
                pretty_err << "Failed to upload file, timeout";
                return;
            }
        }

        // 上传文件
        enableLoss();
        this->sendtoPeer(m_repo.string() + ::std::string(file_list[file_num]));
        disableLoss();

        pretty_log << ::std::format("Upload file \"{}\" successfully to {}", file_list[file_num], this->m_peer.toString());
    }

    template <class Transceiver>
    void RDT_Client<Transceiver>::handle_download()
    {
        // 先从服务器获取文件信息列表
        // 然后选择要下载的文件
        // 选择文件后，发送下载请求

        // 获取文件列表
        pretty_log << ::std::format("Fetching file list from server {}...", this->m_peer.toString());

        ::std::vector<::std::string> file_list;
        ::std::vector<::std::string> file_size_list;
        if (!handle_lss(file_list, file_size_list)) {
            return;
        }

        // 选择文件
        pretty_log << "Choose a file to download (input the number):";
        int file_num;
        while (true) {
            file_num = get_num_input();
            if (file_num >= 0 && file_num < file_list.size()) {
                break;
            } else {
                pretty_err << "File number out of range, please input again";
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
        int cnt = 0;
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

    template <class Transceiver>
    inline int RDT_Client<Transceiver>::get_num_input()
    {
        int num;
        while (true) {
            ::std::cout << ">>> ";
            ::std::string num_str;
            ::std::getline(::std::cin, num_str);
            if (::std::all_of(num_str.begin(), num_str.end(), ::isdigit)) {
                num = ::std::stoi(num_str);
                break;
            } else {
                pretty_err << "Invalid input, please input a number";
            }
        }
        return num;
    }

    template <class Transceiver>
    inline void RDT_Client<Transceiver>::show_file_list(const ::std::vector<::std::string> &file_list, const ::std::vector<::std::string> &file_size_list)
    {
        int max_file_name_length = 8;
        for (const auto &file_name : file_list) {
            max_file_name_length = ::std::max(max_file_name_length, (int)file_name.size());
        }

        ::std::cout << "No  Filename" << ::std::string(max_file_name_length - 8, ' ') << "  Size" << ::std::endl;
        for (int i = 0; i < file_list.size(); ++i) {
            ::std::cout << ::std::vformat(::std::format("{{:<4}}{{:<{}}}", max_file_name_length), ::std::make_format_args(i, file_list[i])) << "  " << file_size_list[i] << ::std::endl;
        }
    }

} // namespace my

#endif // _RDT_CLIENT_HPP_