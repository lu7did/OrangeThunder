//*--------------------------------------------------------------------------------------------------
//* Callback timer System Class   (HEADER CLASS)
//*--------------------------------------------------------------------------------------------------
//* this is part of the PixiePi platform
//* Only for ham radio usages
//* Copyright 2018,2020 Dr. Pedro E. Colla (LU7DID)
//*--------------------------------------------------------------------------------------------------
#ifndef CallBackTimer_h
#define CallBackTimer_h

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

//--------------------------[Timer Interrupt Class]-------------------------------------------------
// Implements a timer tick class calling periodically a function 
// Function passed as the upcall() needs to be implemented
//--------------------------------------------------------------------------------------------------
class CallBackTimer
{
public:

    CallBackTimer()
    :_execute(false)
    {}

    ~CallBackTimer() {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };
    }

    void stop()
    {

        _execute.store(false, std::memory_order_release);
        if( _thd.joinable() )
            _thd.join();
    }

    void start(int interval, std::function<void(void)> upcall)
    {
        if( _execute.load(std::memory_order_acquire) ) {
            stop();
        };

        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, upcall]()
        {
            while (_execute.load(std::memory_order_acquire)) {
                upcall();
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                //std::this_thread::sleep_for(std::chrono::microseconds(interval));

            }
        });
    }

    bool is_running() const noexcept {
        return ( _execute.load(std::memory_order_acquire) && 
                 _thd.joinable() );
    }

private:
    std::atomic<bool> _execute;
    std::thread _thd;
};
#endif
