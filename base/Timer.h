#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <functional>
#include <memory>

class Timer : noncopyable {
public:
    using TimerCallback = std::function<void()>;

    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          canceled_(false) {}

    void run() const {
        callback_();
    }

    void cancel() { canceled_ = true; }

    bool repeat() const { return repeat_&&!canceled_; }
    Timestamp expiration() const { return expiration_; }
    double interval() const { return interval_; }

    void restart(Timestamp now);

    struct Hash {
        size_t operator()(const std::shared_ptr<Timer>& timer) const {
            return reinterpret_cast<size_t>(timer.get());
        }
    };

    struct Equal {
        bool operator()(const std::shared_ptr<Timer>& a,
                        const std::shared_ptr<Timer>& b) const {
            return a.get() == b.get();
        }
    };

private:
    TimerCallback callback_;
    Timestamp expiration_;
    double interval_;
    bool repeat_;
    bool canceled_;
};