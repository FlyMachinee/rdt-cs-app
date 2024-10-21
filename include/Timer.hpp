#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>

namespace my
{
    /**
     * @brief Timer类用于计时
     */
    class Timer
    {
    public:
        Timer() : m_is_running(false) {}
        ~Timer() = default;

        /**
         * @brief 设置倒计时时长，并开始计时（逻辑上）
         * @param milliseconds 超时时间，单位为毫秒
         */
        void setTimeout(int milliseconds)
        {
            m_is_running = true;
            m_bound = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
        }

        /**
         * @brief 判断是否超时（逻辑上）
         * @return 如果超时则返回true，否则返回false
         */
        bool isTimeout() { return m_is_running && (std::chrono::system_clock::now() >= m_bound); }

        /**
         * @brief 停止计时（逻辑上）
         */
        void stop() { m_is_running = false; }

    private:
        std::chrono::time_point<std::chrono::system_clock> m_bound;
        bool m_is_running;
    };
}

#endif // _TIMER_H_