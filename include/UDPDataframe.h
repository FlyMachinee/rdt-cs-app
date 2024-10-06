#ifndef _UDP_DATAFRAME_H_
#define _UDP_DATAFRAME_H_

#include "./Entity.hpp"

namespace my
{
    class UDPFileReader;

    class UDPDataframe
    {
    public:
        enum Type : char {
            NONE = 0,
            CMD = 1,
            DATA = 4,
            ACK = 20,
        };
        static constexpr int MAX_DATA_SIZE = 1024;
        static constexpr int MAX_SIZE = MAX_DATA_SIZE + 4;

        UDPDataframe();
        UDPDataframe(const char *buffer, int recv_size);
        UDPDataframe(const UDPDataframe &);
        UDPDataframe(UDPDataframe &&) noexcept;
        UDPDataframe &operator=(const UDPDataframe &);
        UDPDataframe &operator=(UDPDataframe &&) noexcept;
        ~UDPDataframe();

        void setType(Type type);
        bool isValid() const noexcept;
        bool isAck() const noexcept;
        bool isAck(char ack_num) const noexcept;
        bool isData() const noexcept;
        bool isCmd() const noexcept;

        const char *data(int &data_size) const;
        const char *cmd() const;
        char getDataNum() const;
        void setDataNum(char data_num);
        char getAckNum() const;
        void setAckNum(char ack_num);

        friend UDPDataframe UDPAck(char ack_num);
        friend UDPDataframe UDPData(char data_num, const char *data, int data_size);
        friend UDPDataframe UDPCmd(::std::string_view cmd);
        friend UDPDataframe recvUDPDataframeFrom(const Host &host, Peer &peer_from);
        friend void sendUDPDataframeTo(const UDPDataframe &dataframe, const Host &host, const Peer &peer_to);
        // friend class UDPFileReaderIterator;
        friend class UDPFileReader;

    private:
        char *m_data;
        int m_size;
    };

    UDPDataframe UDPAck(char ack_num);
    UDPDataframe UDPData(char data_num, const char *data, int data_size);
    UDPDataframe UDPCmd(::std::string_view cmd);

    UDPDataframe recvUDPDataframeFrom(const Host &host, Peer &peer_from);
    void sendUDPDataframeTo(const UDPDataframe &dataframe, const Host &host, const Peer &peer_to);
    char recvAckFrom(const Host &host, Peer &peer_from);
    void sendAckTo(char ack_num, const Host &host, const Peer &peer_to);
    ::std::string recvCmdFrom(const Host &host, Peer &peer_from);
    void sendCmdTo(::std::string_view cmd, const Host &host, const Peer &peer_to);
} // namespace my

#endif // _UDP_DATAFRAME_H_