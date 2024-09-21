
#include <format>

#include "../include/UDPFileWriter.h"
#include "../include/pretty_log.hpp"

my::UDPFileWriter::UDPFileWriter(::std::string_view filename) : m_filename(filename)
{
    m_ofs.open(m_filename, ::std::ios::binary | ::std::ios::app);
    if (!m_ofs.is_open()) {
        pretty_out << ::std::format("throw from UDPWriteFile::UDPWriteFile(): Failed to open file {0}", m_filename);
        throw std::runtime_error("Failed to open file");
    }
}

my::UDPFileWriter::~UDPFileWriter()
{
    close();
}

void my::UDPFileWriter::append(const UDPDataframe &dataframe)
{
    if (!m_ofs.is_open()) {
        pretty_out << "throw from UDPWriteFile::append(): File is not open";
        throw std::runtime_error("File is not open");
    }

    int data_size;
    const char *data = dataframe.data(data_size);
    m_ofs.write(data, data_size);
}

void my::UDPFileWriter::close()
{
    m_ofs.flush();
    m_ofs.close();
}