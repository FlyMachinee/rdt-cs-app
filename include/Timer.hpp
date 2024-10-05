#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>

namespace my
{
    class Timer
    {
    public:
        Timer() : m_is_running(false) {}
        ~Timer() = default;

        void setTimeout(int milliseconds)
        {
            m_is_running = true;
            m_bound = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
        }

        bool isTimeout() { return m_is_running && (std::chrono::system_clock::now() >= m_bound); }

        void stop() { m_is_running = false; }

    private:
        std::chrono::time_point<std::chrono::system_clock> m_bound;
        bool m_is_running;
    };
}

#endif // _TIMER_H_