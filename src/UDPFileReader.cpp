#include "../include/UDPFileReader.h"
#include "../include/pretty_log.hpp"

#include <format>

my::UDPFileReader::UDPFileReader(::std::string_view filename)
{
    m_ifs.open(filename.data(), ::std::ios::binary);
    if (!m_ifs.is_open()) {
        pretty_out << ::std::format("throw from UDPFile::UDPFile(): Failed to open file {0}", filename);
        throw std::runtime_error("Failed to open file");
    }
}

my::UDPFileReader::~UDPFileReader()
{
    close();
}

void my::UDPFileReader::close()
{
    m_ifs.close();
}

my::UDPFileReader::iterator my::UDPFileReader::begin()
{
    return UDPFileReaderIterator(*this, false);
}

my::UDPFileReader::iterator my::UDPFileReader::end()
{
    return UDPFileReaderIterator(*this, true);
}

my::UDPFileReaderIterator::UDPFileReaderIterator(UDPFileReader &udpfile, bool is_end) : m_udpfile(udpfile)
{
    if (!udpfile.m_ifs.is_open()) {
        pretty_out << "throw from UDPFileIterator::UDPFileIterator(): File is not open";
        throw std::runtime_error("File is not open");
    }

    udpfile.m_ifs.seekg(0, ::std::ios::end);
    m_file_size = udpfile.m_ifs.tellg();
    udpfile.m_ifs.seekg(0, ::std::ios::beg);

    m_max_line = (m_file_size + UDPDataframe::MAX_DATA_SIZE - 1) / UDPDataframe::MAX_DATA_SIZE;
    if (is_end) {
        m_line = m_max_line;
    } else {
        m_line = 0;
    }
}

my::UDPFileReaderIterator::~UDPFileReaderIterator()
{
    m_udpfile.m_ifs.seekg(0, ::std::ios::beg);
}

bool my::UDPFileReaderIterator::operator!=(const UDPFileReaderIterator &other) const
{
    bool is_end = m_line >= m_max_line;
    bool other_is_end = other.m_line >= other.m_max_line;
    return (&m_udpfile != &other.m_udpfile) || ((m_line != other.m_line) && !(is_end && other_is_end));
}

my::UDPFileReaderIterator &my::UDPFileReaderIterator::operator++()
{
    ++m_line;
    return *this;
}

::std::pair<my::UDPDataframe, int> my::UDPFileReaderIterator::operator*()
{
    m_udpfile.m_ifs.seekg(m_line * UDPDataframe::MAX_DATA_SIZE, ::std::ios::beg);
    UDPDataframe dataframe;
    dataframe.m_data[0] = UDPDataframe::DATA;
    dataframe.m_data[1] = (char)0;
    int can_get_size = (m_line < m_max_line) ? UDPDataframe::MAX_DATA_SIZE : m_file_size % UDPDataframe::MAX_DATA_SIZE;
    *reinterpret_cast<short *>(dataframe.m_data + 2) = (short)can_get_size;
    m_udpfile.m_ifs.read(dataframe.m_data + 4, can_get_size);
    dataframe.m_size = can_get_size + 4;
    return {dataframe, m_line};
}
