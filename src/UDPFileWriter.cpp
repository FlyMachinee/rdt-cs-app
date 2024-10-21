#include <format>

#include "../include/UDPFileWriter.h"
#include "../include/pretty_log.hpp"

/**
 * @brief 构造一个能够将UDP数据分组写入文件的UDPFileWriter对象
 */
my::UDPFileWriter::UDPFileWriter(::std::string_view filename)
{
    m_ofs.open(filename.data(), ::std::ios::binary | ::std::ios::app);
    if (!m_ofs.is_open()) {
        pretty_out << ::std::format("throw from UDPFileWriter::UDPFileWriter(): Failed to open file \"{0}\"", filename);
        throw std::runtime_error("Failed to open file");
    }
}

/**
 * @brief 析构函数
 */
my::UDPFileWriter::~UDPFileWriter()
{
    close();
}

/**
 * @brief 将一个UDP数据分组追加写入文件
 */
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

/**
 * @brief 关闭文件
 */
void my::UDPFileWriter::close()
{
    m_ofs.flush();
    m_ofs.close();
}