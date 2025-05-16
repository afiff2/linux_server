#pragma once

#include "Timer.h"

class TimerId
{
    public:
        TimerId(Timer *timer) : timer_(timer) {}
        
        Timer *getTimer() const { return timer_; }

    private:
        Timer *timer_;
};