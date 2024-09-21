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