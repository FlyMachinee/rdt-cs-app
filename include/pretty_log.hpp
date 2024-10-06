#ifndef _PRETTY_LOG_HPP
#define _PRETTY_LOG_HPP

#include <iostream>
#include <mutex>
#include <string>

namespace my
{
    inline std::mutex g_log_mutex;

    class pretty_wapper
    {
    public:
        pretty_wapper(::std::ostream &os, const ::std::string &prefix) : m_os(os), m_prefix(prefix)
        {
            valid = true;
            m_is_first = true;
            g_log_mutex.lock();
        }

        pretty_wapper(const pretty_wapper &) = delete;
        pretty_wapper &operator=(const pretty_wapper &) = delete;

        pretty_wapper(pretty_wapper &&other) : m_os(other.m_os), m_prefix(other.m_prefix), m_is_first(other.m_is_first), valid(other.valid)
        {
            other.valid = false;
        }
        pretty_wapper &operator=(pretty_wapper &&) = delete;

        ~pretty_wapper()
        {
            if (!valid) {
                return;
            }
            m_os << ::std::endl;
            g_log_mutex.unlock();
        }

        template <typename T>
        pretty_wapper &operator<<(const T &t)
        {
            if (!valid) {
                return *this;
            }

            if (m_is_first) {
                m_is_first = false;
                m_os << m_prefix << t;
            } else {
                m_os << '\n'
                     << ::std::string(m_prefix.length(), ' ') << t;
            }
            return *this;
        }

        pretty_wapper &operator<<(std::ostream &(*pf)(std::ostream &))
        {
            if (!valid) {
                return *this;
            }

            m_os << pf;
            return *this;
        }

    private:
        ::std::ostream &m_os;
        ::std::string m_prefix;
        bool m_is_first;
        bool valid;
    };

    class pretty_wapper_wapper
    {
    public:
        pretty_wapper_wapper(::std::ostream &os, const ::std::string &prefix) : m_os(os), m_prefix(prefix) {}

        template <typename T>
        pretty_wapper operator<<(const T &t)
        {
            return ::std::move(pretty_wapper(m_os, m_prefix) << t);
        }

    private:
        ::std::ostream &m_os;
        ::std::string m_prefix;
    };

    inline pretty_wapper_wapper pretty_log = pretty_wapper_wapper(::std::clog, "[log] ");
    inline pretty_wapper_wapper pretty_log_con = pretty_wapper_wapper(::std::cout, "      ");
    inline pretty_wapper_wapper pretty_err = pretty_wapper_wapper(::std::cerr, "[error] ");
    inline pretty_wapper_wapper pretty_err_con = pretty_wapper_wapper(::std::cout, "        ");
    inline pretty_wapper_wapper pretty_out = pretty_wapper_wapper(::std::cout, "");
}

#endif // _PRETTY_LOG_HPP