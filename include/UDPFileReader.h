
#include <fstream>
#include <string_view>

#include "./UDPDataframe.h"

namespace my
{
    class UDPFileReaderIterator;

    class UDPFileReader
    {
    public:
        friend class UDPFileReaderIterator;
        using iterator = UDPFileReaderIterator;

        UDPFileReader(::std::string_view filename);
        ~UDPFileReader();

        void close();

        iterator begin();
        iterator end();

    private:
        ::std::string m_filename;
        ::std::ifstream m_ifs;
    };

    class UDPFileReaderIterator
    {
    public:
        UDPFileReaderIterator(UDPFileReader &udpfile, bool is_end);
        ~UDPFileReaderIterator();

        bool operator!=(const UDPFileReaderIterator &other) const;
        UDPFileReaderIterator &operator++();
        ::std::pair<UDPDataframe, int> operator*();

    private:
        UDPFileReader &m_udpfile;
        int m_line;
        int m_max_line;
        int m_file_size;
    };
} // namespace my
