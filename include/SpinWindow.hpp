#ifndef _SPIN_WINDOW_HPP_
#define _SPIN_WINDOW_HPP_

#include <algorithm>
#include <type_traits>

#include "./Timer.hpp"
#include "./UDPFileWriter.h"

namespace my
{
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

        int getBegin() const noexcept { return begin; }
        void clear() noexcept
        {
            ::std::fill(arr, arr + seqNumBound, false);
            begin = 0;
        }

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

        bool submit(int seq_num) noexcept
        {
            if (canSubmit(seq_num)) {
                arr[seq_num] = true;
                return true;
            }
            return false;
        }

        int howMuchCanSpin() const noexcept
        {
            int ret = 0;
            int end = (begin + windowSize) % seqNumBound;
            for (int i = begin; i != end && arr[i]; i = (i + 1) % seqNumBound)
                ++ret;
            return ret;
        }

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

    template <int windowSize, int seqNumBound>
        requires(windowSize <= seqNumBound - 1 && windowSize > 0)
    class SpinWindowWithTimer : public SpinWindow<windowSize, seqNumBound>
    {
    public:
        SpinWindowWithTimer() noexcept = default;
        SpinWindowWithTimer(int begin) noexcept
            : SpinWindow<windowSize, seqNumBound>(begin) {}
        virtual ~SpinWindowWithTimer() = default;

        void timerStop(int seq_num)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            timerArr[seq_num].stop();
        }

        void timerStopAll()
        {
            for (int i = 0; i < seqNumBound; ++i)
                timerArr[i].stop();
        }

        void clear() noexcept
        {
            SpinWindow<windowSize, seqNumBound>::clear();
            timerStopAll();
        }

        void timerSetTimeout(int seq_num, int milliseconds)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            timerArr[seq_num].setTimeout(milliseconds);
        }

        bool timerIsTimeout(int seq_num)
        {
            if (seq_num < 0 || seq_num >= seqNumBound)
                throw ::std::out_of_range("seq_num out of range");
            return timerArr[seq_num].isTimeout();
        }

        int whichTimerIsTimeout()
        {
            for (int i = 0; i < seqNumBound; ++i)
                if (timerArr[i].isTimeout())
                    return i;
            return -1;
        }

        bool submit(int seq_num)
        {
            if (this->canSubmit(seq_num)) {
                timerArr[seq_num].stop();
                this->arr[seq_num] = true;
                return true;
            }
            return false;
        }

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