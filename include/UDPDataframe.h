
#include <functional>
#include <string_view>

#include "./Entity.hpp"

namespace my
{
    class UDPDataframe
    {
    public:
        static constexpr int MAX_DATA_SIZE = 1024;
        static constexpr int MAX_SIZE = MAX_DATA_SIZE + 4;
        static constexpr int ACK = 20;
        static constexpr int DATA = 4;
        static constexpr int NONE = 0;

        UDPDataframe();
        UDPDataframe(const char *buffer, int recv_size);
        virtual ~UDPDataframe() = default;

        bool isAck() const noexcept;
        bool isData() const noexcept;

        const char *data(int &data_size) const;

        int getDataNum() const;
        void setDataNum(int data_num);
        void modDataNum(int modulus);
        void transDataNum(::std::function<int(int)> f);

        int getAckNum() const;

        friend UDPDataframe UDPAck(int ack_num);
        friend UDPDataframe UDPData(int data_num, const char *data, int data_size);
        friend UDPDataframe recvUDPDataframeFrom(const Host &host, Peer &peer_from);
        friend void sendUDPDataframeTo(const UDPDataframe &dataframe, const Host &host, const Peer &peer_to);
        friend class UDPFileReaderIterator;

    private:
        char m_data[MAX_SIZE + 1];
        int m_size;
    };

    UDPDataframe UDPAck(int ack_num);
    UDPDataframe UDPData(int data_num, const char *data, int data_size);

    UDPDataframe recvUDPDataframeFrom(const Host &host, Peer &peer_from);
    void sendUDPDataframeTo(const UDPDataframe &dataframe, const Host &host, const Peer &peer_to);
} // namespace my
