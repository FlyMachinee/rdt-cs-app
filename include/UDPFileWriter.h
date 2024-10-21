#ifndef _UDP_FILE_WRITER_H_
#define _UDP_FILE_WRITER_H_

#include "./UDPDataframe.h"

#include <fstream>
#include <string_view>

namespace my
{
    /**
     * @brief 用于将UDP数据分组写入文件的类
     */
    class UDPFileWriter
    {
    public:
        UDPFileWriter(::std::string_view filename);
        ~UDPFileWriter();

        void append(const UDPDataframe &dataframe);
        void close();

    private:
        ::std::ofstream m_ofs;
    };
} // namespace my

#endif // _UDP_FILE_WRITER_H_