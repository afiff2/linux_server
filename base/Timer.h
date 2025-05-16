#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>

class Timer : noncopyable {
public:
    using TimerCallback = std::function<void()>;

    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0) {}

    void run() const {
        callback_();
    }

    bool repeat() const { return repeat_; }
    Timestamp expiration() const { return expiration_; }
    double interval() const { return interval_; }

    void restart(Timestamp now);

private:
    TimerCallback callback_;
    Timestamp expiration_;
    double interval_;
    bool repeat_;
};