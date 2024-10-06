#include "../include/UDPFileReader.h"
#include "../include/pretty_log.hpp"

#include <format>

::my::UDPFileReader::UDPFileReader(::std::string_view filename)
{
    m_ifs.open(filename.data(), ::std::ios::binary);
    if (!m_ifs.is_open()) {
        pretty_out << ::std::format("throw from UDPFileReader::UDPFileReader(): Failed to open file \"{0}\"", filename);
        throw std::runtime_error("Failed to open file");
    }

    m_ifs.seekg(0, ::std::ios::end);
    m_file_size = m_ifs.tellg();
    m_block_count = (m_file_size + UDPDataframe::MAX_DATA_SIZE - 1) / UDPDataframe::MAX_DATA_SIZE;
    m_ifs.seekg(0, ::std::ios::beg);
}

::my::UDPFileReader::~UDPFileReader()
{
    close();
}

void ::my::UDPFileReader::close()
{
    m_ifs.close();
}

int ::my::UDPFileReader::getBlockCount()
{
    return m_block_count;
}

::my::UDPDataframe my::UDPFileReader::getDataframe(int block_num)
{
    if (block_num < 0 || block_num > m_block_count) {
        pretty_out << ::std::format("throw from UDPFile::getDataframe(): Invalid block_num, block_num = {0}", block_num);
        throw std::runtime_error("Invalid block_num");
    }

    UDPDataframe dataframe;
    dataframe.m_data[0] = UDPDataframe::DATA;
    dataframe.m_data[1] = (char)0;

    // 如果是最后一个数据块
    if (block_num == m_block_count) {
        // 发送一个空数据块
        *reinterpret_cast<short *>(dataframe.m_data + 2) = 0;
        dataframe.m_size = 4;
    } else {
        // 发送一个正常的数据块
        m_ifs.seekg(block_num * UDPDataframe::MAX_DATA_SIZE, ::std::ios::beg);
        int can_get_size = (block_num < m_block_count - 1) ? UDPDataframe::MAX_DATA_SIZE : m_file_size % UDPDataframe::MAX_DATA_SIZE;
        *reinterpret_cast<short *>(dataframe.m_data + 2) = (short)can_get_size;
        m_ifs.read(dataframe.m_data + 4, can_get_size);
        dataframe.m_size = can_get_size + 4;
    }
    return dataframe;
}

// my::UDPFileReader::iterator my::UDPFileReader::begin()
// {
//     return UDPFileReaderIterator(*this, false);
// }

// my::UDPFileReader::iterator my::UDPFileReader::end()
// {
//     return UDPFileReaderIterator(*this, true);
// }

// my::UDPFileReaderIterator::UDPFileReaderIterator(UDPFileReader &udpfile, bool is_end) : m_udpfile(udpfile)
// {
//     if (!udpfile.m_ifs.is_open()) {
//         pretty_out << "throw from UDPFileIterator::UDPFileIterator(): File is not open";
//         throw std::runtime_error("File is not open");
//     }

//     udpfile.m_ifs.seekg(0, ::std::ios::end);
//     m_file_size = udpfile.m_ifs.tellg();
//     udpfile.m_ifs.seekg(0, ::std::ios::beg);

//     m_max_line = (m_file_size + UDPDataframe::MAX_DATA_SIZE - 1) / UDPDataframe::MAX_DATA_SIZE;
//     if (is_end) {
//         m_line = m_max_line;
//     } else {
//         m_line = 0;
//     }
// }

// my::UDPFileReaderIterator::~UDPFileReaderIterator()
// {
//     m_udpfile.m_ifs.seekg(0, ::std::ios::beg);
// }

// bool my::UDPFileReaderIterator::operator!=(const UDPFileReaderIterator &other) const
// {
//     bool is_end = m_line >= m_max_line;
//     bool other_is_end = other.m_line >= other.m_max_line;
//     return (&m_udpfile != &other.m_udpfile) || ((m_line != other.m_line) && !(is_end && other_is_end));
// }

// my::UDPFileReaderIterator &my::UDPFileReaderIterator::operator++()
// {
//     ++m_line;
//     return *this;
// }

// ::std::pair<my::UDPDataframe, int> my::UDPFileReaderIterator::operator*()
// {
//     m_udpfile.m_ifs.seekg(m_line * UDPDataframe::MAX_DATA_SIZE, ::std::ios::beg);
//     UDPDataframe dataframe;
//     dataframe.m_data[0] = UDPDataframe::DATA;
//     dataframe.m_data[1] = (char)0;
//     int can_get_size = (m_line < m_max_line) ? UDPDataframe::MAX_DATA_SIZE : m_file_size % UDPDataframe::MAX_DATA_SIZE;
//     *reinterpret_cast<short *>(dataframe.m_data + 2) = (short)can_get_size;
//     m_udpfile.m_ifs.read(dataframe.m_data + 4, can_get_size);
//     dataframe.m_size = can_get_size + 4;
//     return {dataframe, m_line};
// }
