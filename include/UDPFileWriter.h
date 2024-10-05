#ifndef _UDP_FILE_WRITER_H_
#define _UDP_FILE_WRITER_H_

#include "./UDPDataframe.h"

#include <fstream>
#include <string_view>

namespace my
{
    class UDPFileWriter
    {
    public:
        UDPFileWriter(::std::string_view filename);
        ~UDPFileWriter();

        void append(const UDPDataframe &dataframe);
        void close();

    private:
        ::std::string m_filename;
        ::std::ofstream m_ofs;
    };
} // namespace my

#endif // _UDP_FILE_WRITER_H_