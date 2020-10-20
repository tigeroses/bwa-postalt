
#pragma once

#include <mutex>
#include <thread>

class Timer
{
public:
    Timer() : t1(res::zero()), t2(res::zero())
    {
        tic();
    }

    ~Timer() {}

    void tic()
    {
        t1 = clock::now();
    }

    // Set div as default 1, the time unit is millisecond.
    // If set div as 1000, the time unit is second
    double toc(int div = 1)
    {
        t2              = clock::now();
        double duration = std::chrono::duration_cast< res >(t2 - t1).count() / 1e3;
        duration /= div;
        if (duration < 0.01)
            duration = 0.0;
        tic();
        return duration;
    }

private:
    typedef std::chrono::high_resolution_clock clock;
    typedef std::chrono::microseconds          res;

    clock::time_point t1;
    clock::time_point t2;
};
