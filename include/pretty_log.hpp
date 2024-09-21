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
            g_log_mutex.lock();
            m_os << m_prefix;
            m_is_first = true;
        }

        ~pretty_wapper()
        {
            g_log_mutex.unlock();
        }

        template <typename T>
        pretty_wapper &operator<<(const T &t)
        {
            if (m_is_first) {
                m_is_first = false;
                m_os << t << '\n';
            } else {
                m_os << ::std::string(m_prefix.length(), ' ') << t << '\n';
            }
            return *this;
        }

        pretty_wapper &operator<<(std::ostream &(*pf)(std::ostream &))
        {
            m_os << pf;
            return *this;
        }

    private:
        ::std::ostream &m_os;
        ::std::string m_prefix;
        bool m_is_first;
    };

    class pretty_wapper_wapper
    {
    public:
        pretty_wapper_wapper(::std::ostream &os, const ::std::string &prefix) : m_os(os), m_prefix(prefix) {}

        template <typename T>
        pretty_wapper operator<<(const T &t)
        {
            return pretty_wapper(m_os, m_prefix) << t;
        }

    private:
        ::std::ostream &m_os;
        ::std::string m_prefix;
    };

    inline pretty_wapper_wapper pretty_log = pretty_wapper_wapper(::std::clog, "[log] ");
    inline pretty_wapper_wapper pretty_err = pretty_wapper_wapper(::std::cerr, "[err] ");
    inline pretty_wapper_wapper pretty_out = pretty_wapper_wapper(::std::cout, "");
}

#endif // _PRETTY_LOG_HPP