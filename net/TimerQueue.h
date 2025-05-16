#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include "Channel.h"
#include "Timer.h"

#include <memory>
#include <set>

class TimerId;
class EventLoop;

class TimerQueue : noncopyable
{
    public:
        TimerQueue(EventLoop* loop);
        ~TimerQueue();

        TimerId addTimer(const Timer::TimerCallback& cb, Timestamp when, double interval);

        //void cancel(TimerId timerId);

    private:
        using Entry = std::pair<Timestamp, std::unique_ptr<Timer>>;
        using TimerSet = std::set<Entry>;

        void handleRead();

        //move out all expired timers
        std::vector<Entry> getExpired(Timestamp now);
        void reset(const std::vector<Entry>& expired, Timestamp now);

        bool insert(std::unique_ptr<Timer> timer);

        void updateTimerFd(Timestamp expiration);

        EventLoop* loop_;
        const int timerfd_;
        Channel timerfdChannel_;
        TimerSet timers_;
};