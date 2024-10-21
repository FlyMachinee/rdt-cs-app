#ifndef _SPIN_WINDOW_HPP_
#define _SPIN_WINDOW_HPP_

#include <algorithm>
#include <type_traits>

#include "./Timer.hpp"
#include "./UDPFileWriter.h"

namespace my
{

    /**
     * @brief 旋转窗口类，专门为SR协议设计。封装了一个长度为序号数seqNumBound的数组
     * @tparam windowSize 窗口大小
     * @tparam seqNumBound 序号数上界
     */
    template <int windowSize, int seqNumBound>
        requires(windowSize <= seqNumBound - 1 && windowSize > 0)
    class SpinWindow
    {
    public:
        SpinWindow() noexcept
            : begin(0) {}
        SpinWindow(int begin) noexcept
            : begin(begin) {}
        virtual ~SpinWindow() = default;

        /**
         * @brief 获取当前窗口的起始编号(base)
         * @return 窗口的起始编号
         */
        int getBegin() const noexcept { return begin; }

        /**
         * @brief 恢复窗口至初始状态
         */
        void clear() noexcept
        {
            ::std::fill(arr, arr + seqNumBound, false);
            begin = 0;
        }

        /**
         * @brief 判断序号seq_num是否可以提交（处于当前窗口之内且未提交过）
         * @param seq_num 序号
         * @return 如果可以提交则返回true，否则返回false
         */
        bool canSubmit(int seq_num) const noexcept
        {
            if (seq_num < 0 || seq_num >= seqNumBound || arr[seq_num])
                return false;

            int end = (begin + windowSize) % seqNumBound;
            if (begin < end)
                return seq_num >= begin && seq_num < end;
            else
                return seq_num >= begin || seq_num < end;
        }

        /**
         * @brief 提交序号seq_num（将其对应位标记为已提交）
         * @param seq_num 序号
         * @return 如果提交成功则返回true，否则返回false
         */
        bool submit(int seq_num) noexcept
        {
            if (canSubmit(seq_num)) {
                arr[seq_num] = true;
                return true;
            }
            return false;
        }

        /**
         * @brief 获取当前窗口可以向前滑动的步长
         * @return 可以向前滑动的步长
         */
        int howMuchCanSpin() const noexcept
        {
            int ret = 0;
            int end = (begin + windowSize) % seqNumBound;
            for (int i = begin; i != end && arr[i]; i = (i + 1) % seqNumBound)
                ++ret;
            return ret;
        }

        /**
         * @brief 将窗口进行向前滑动
         * @param spin_cnt 滑动的步长
         * @return 如果滑动成功则返回true，否则返回false
         */
        bool spin(int spin_cnt) noexcept
        {
            if (spin_cnt < 0 || spin_cnt > howMuchCanSpin())
                return false;
            while (spin_cnt--) {
                arr[begin] = false;
                begin = (begin + 1) % seqNumBound;
            }
            return true;
        }

        /**
         * @brief 将窗口进行向前滑动，直到遇到第一个未提交的序号
         * @return 滑动的步长
         */
        int spin() noexcept
        {
            int ret = 0;
            while (arr[begin]) {
                arr[begin] = false;
                begin = (begin + 1) % seqNumBound;
                ++ret;
            }
            return ret;
        }

    protected:
        bool arr[seqNumBound] = {false};
        int begin;
    };

    /**
     * @brief 带缓存的旋转窗口类，专门为SR协议接收方设计。封装了一个长度为序号数seqNumBound的数组和一个长度为seqNumBound的缓存数组
     * @tparam windowSize 窗口大小
     * @tparam seqNumBound 序号数上界
     * @tparam DataType 缓存数据类型
     */
    template <int windowSize, int seqNumBound, class DataType>
        requires(
            windowSize <= seqNumBound - 1 &&
            windowSize > 0 &&
            ::std::is_default_constructible_v<DataType>)
    class SpinWindowWithCache : public SpinWindow<windowSize, seqNumBound>
    {
    public:
        SpinWindowWithCache() noexcept = default;
        SpinWindowWithCache(int begin) noexcept
            : SpinWindow<windowSize, seqNumBound>(begin) {}
        virtual ~SpinWindowWithCache() = default;

        /**
         * @brief 提交序号seq_num和数据data，进行拷贝构造
         * @param seq_num 序号
         * @param data 数据
         * @return 如果提交成功则返回true，否则返回false
         */
        bool submit(int seq_num, const DataType &data) noexcept
            requires(::std::is_copy_constructible_v<DataType>)
        {
            if (this->canSubmit(seq_num)) {
                cacheArr[seq_num] = data;
                this->arr[seq_num] = true;
                return true;
            }
            return false;
        }

        /**
         * @brief 提交序号seq_num和数据data，进行移动构造
         * @param seq_num 序号
         * @param data 数据
         * @return 如果提交成功则返回true，否则返回false
         */
        bool submit(int seq_num, DataType &&data) noexcept
            requires(::std::is_move_constructible_v<DataType>)
        {
            if (this->canSubmit(seq_num)) {
                cacheArr[seq_num] = ::std::move(data);
                this->arr[seq_num] = true;
                return true;
            }
            return false;
        }

        /**
         * @brief 将缓存中的数据提交到writer中，并且将窗口向前滑动
         * @param writer UDPFileWriter对象
         * @return 成功提交的数据个数
         */
        int spin(UDPFileWriter &writer)
        {
            int ret = 0;
            while (this->arr[this->begin]) {
                writer.append(cacheArr[this->begin]);
                this->arr[this->begin] = false;
                this->begin = (this->begin + 1) % seqNumBound;
                ++ret;
            }
            return ret;
        }

        DataType &operator[](int seq_num) noexcept { return cacheArr[seq_num]; }
        DataType &at(int seq_num) noexcept
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            return cacheArr[seq_num];
        }

    protected:
        using SpinWindow<windowSize, seqNumBound>::submit;
        using SpinWindow<windowSize, seqNumBound>::spin;

        DataType cacheArr[seqNumBound];
    };

    /**
     * @brief 带定时器的旋转窗口类，专门为SR发送方协议设计。封装了一个长度为序号数seqNumBound的数组和一个长度为seqNumBound的定时器数组
     * @tparam windowSize 窗口大小
     * @tparam seqNumBound 序号数上界
     */
    template <int windowSize, int seqNumBound>
        requires(windowSize <= seqNumBound - 1 && windowSize > 0)
    class SpinWindowWithTimer : public SpinWindow<windowSize, seqNumBound>
    {
    public:
        SpinWindowWithTimer() noexcept = default;
        SpinWindowWithTimer(int begin) noexcept
            : SpinWindow<windowSize, seqNumBound>(begin) {}
        virtual ~SpinWindowWithTimer() = default;

        /**
         * @brief 关闭序号seq_num对应的定时器
         * @param seq_num 序号
         */
        void timerStop(int seq_num)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            timerArr[seq_num].stop();
        }

        /**
         * @brief 关闭所有定时器
         */
        void timerStopAll()
        {
            for (int i = 0; i < seqNumBound; ++i)
                timerArr[i].stop();
        }

        /**
         * @brief 恢复窗口与所有计时器至初始状态
         */
        void clear() noexcept
        {
            SpinWindow<windowSize, seqNumBound>::clear();
            timerStopAll();
        }

        /**
         * @brief 设置序号seq_num对应的定时器的超时时间
         * @param seq_num 序号
         * @param milliseconds 超时时间，单位为毫秒
         */
        void timerSetTimeout(int seq_num, int milliseconds)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            timerArr[seq_num].setTimeout(milliseconds);
        }

        /**
         * @brief 判断序号seq_num对应的定时器是否超时
         * @param seq_num 序号
         * @return 如果超时则返回true，否则返回false
         */
        bool timerIsTimeout(int seq_num)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            return timerArr[seq_num].isTimeout();
        }

        /**
         * @brief 获取某一个超时的定时器的序号
         * @return 超时的定时器的序号，如果没有超时则返回-1
         */
        int whichTimerIsTimeout()
        {
            for (int i = 0; i < seqNumBound; ++i)
                if (timerArr[i].isTimeout())
                    return i;
            return -1;
        }

        /**
         * @brief 提交序号seq_num（将其对应位标记为已提交）并关闭其对应的定时器
         * @param seq_num 序号
         * @return 如果提交成功则返回true，否则返回false
         */
        bool submit(int seq_num)
        {
            if (this->canSubmit(seq_num)) {
                timerArr[seq_num].stop();
                this->arr[seq_num] = true;
                return true;
            }
            return false;
        }

        /**
         * @brief 将窗口进行向前滑动，并关闭所有已提交序号对应的定时器
         * @return 成功滑动的步长
         */
        int spin()
        {
            int ret = 0;
            while (this->arr[this->begin]) {
                timerArr[this->begin].stop();
                this->arr[this->begin] = false;
                this->begin = (this->begin + 1) % seqNumBound;
                ++ret;
            }
            return ret;
        }

    protected:
        using SpinWindow<windowSize, seqNumBound>::submit;
        using SpinWindow<windowSize, seqNumBound>::spin;
        Timer timerArr[seqNumBound];
    };
} // namespace my

#endif // _SPIN_WINDOW_HPP_