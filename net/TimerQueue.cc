#include "TimerQueue.h"
#include "EventLoop.h"
#include "Logger.h"
#include "TimerId.h"

#include <algorithm>
#include <cassert>
#include <errno.h>
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <functional>

using namespace std::placeholders;

namespace detail
{

struct timespec toTimeSpec(Timestamp when)
{
    struct timespec ts;
    int64_t microseconds = when.microSecondsSinceEpoch();
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

} // namespace detail

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop), timerfd_(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      timerfdChannel_(loop_, timerfd_), callingExpiredTimers_(false)
{
    if (timerfd_ < 0)
    {
        LOG_FATAL << "Failed to create timerfd";
    }

    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() { ::close(timerfd_); }

TimerId TimerQueue::addTimer(const Timer::TimerCallback &cb, Timestamp when, double interval)
{

    std::shared_ptr<Timer> timer = std::make_shared<Timer>(cb, when, interval);

    loop_->runInLoop([this, timer]() {
        this->addTimerInLoop(timer);
    });

    return TimerId(timer);
}

void TimerQueue::addTimerInLoop(const std::shared_ptr<Timer>& timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        updateTimerFd(timer->expiration());
    }

}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now = Timestamp::now();

    uint64_t one;
    ssize_t n = ::read(timerfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    for (const auto &it : expired)
    {
        it.second->run(); // 执行回调
    }
    

    reset(expired, now);

    callingExpiredTimers_ = false;
    cancelingTimers_.clear();
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;

    Entry sentry = std::make_pair(now, std::shared_ptr<Timer>(nullptr));

    auto end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    expired.assign(timers_.begin(), end);
    timers_.erase(timers_.begin(), end);

    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire = Timestamp::invalid();
    for (const auto &it : expired)
    {
        std::shared_ptr<Timer> timer = it.second;
        if(cancelingTimers_.count(timer))
        {
            LOG_TRACE << "Timer was canceled during callback, not re-adding";
            timer->cancel();
        }
        if (timer->repeat())
        {
            timer->restart(now);
            insert(timer);
        }
    }
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->first;
    }

    if (nextExpire != Timestamp::invalid())
    {
        updateTimerFd(nextExpire);
    }
}

bool TimerQueue::insert(std::shared_ptr<Timer> timer)
{
    Timestamp when = timer->expiration();
    LOG_TRACE << "Adding timer at " << when.secondsSinceEpoch() << " Now is " << Timestamp::now().secondsSinceEpoch();
    bool earliestChanged = false;

    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    std::pair<TimerSet::iterator, bool> result = timers_.insert(std::make_pair(when, timer));
    assert(result.second);

    return earliestChanged;
}

void TimerQueue::updateTimerFd(Timestamp expiration)
{
    int64_t diff = expiration.microSecondsSinceEpoch()
                 - Timestamp::now().microSecondsSinceEpoch();
    if (diff < 100) diff = 100; // 最小 100 μs
    struct itimerspec new_value;
    bzero(&new_value, sizeof new_value);
    new_value.it_value.tv_sec  = diff / Timestamp::kMicroSecondsPerSecond;
    new_value.it_value.tv_nsec = (diff % Timestamp::kMicroSecondsPerSecond) * 1000;
    // 取消绝对模式
    if (::timerfd_settime(timerfd_, /*flags=*/0, &new_value, nullptr) < 0)
    {
        LOG_ERROR << "timerfd_settime failed";
    }
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(
        [this,timerId](){this->cancelInLoop(timerId);}
    );
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();

    auto timer = timerId.timer();
    if (!timer) return;

    auto compare = [timer](const Entry& entry) {
        return entry.second.get() == timer.get();
    };

    for (auto it = timers_.begin(); it != timers_.end(); ++it)
        if (compare(*it))
        {
            LOG_TRACE << "Cancel timer immediately";
            it->second->cancel();
            timers_.erase(it);
            return;
        }
    
    if(callingExpiredTimers_)//timer可能在getExpired函数中
    {
        LOG_TRACE << "Delay cancel timer (in callback)";
        cancelingTimers_.insert(timer);
    }

}