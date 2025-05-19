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
      timerfdChannel_(loop_, timerfd_)
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

    Timer *ti = new Timer(cb, when, interval);

    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, ti));
    return TimerId(ti);
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    loop_->assertInLoopThread();
    std::unique_ptr<Timer> timerPtr(timer);
    bool earliestChanged = insert(std::move(timerPtr));

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
    for (const auto &it : expired)
    {
        it.second->run(); // 执行回调
    }

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;

    Entry sentry = std::make_pair(now, std::unique_ptr<Timer>(nullptr));

    auto end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    for (auto it = timers_.begin(); it != end; ++it)
    {
        expired.emplace_back(std::move(const_cast<Entry &>(*it)));
    }
    timers_.erase(timers_.begin(), end);

    return expired;
}

void TimerQueue::reset(std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire = Timestamp::invalid();
    for (auto &it : expired)
    {
        std::unique_ptr<Timer> timer = std::move(it.second);
        if (timer->repeat())
        {
            timer->restart(now);
            insert(std::move(timer));
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

bool TimerQueue::insert(std::unique_ptr<Timer> timer)
{
    Timestamp when = timer->expiration();
    LOG_TRACE << "Adding timer at " << when.secondsSinceEpoch() << " Now is " << Timestamp::now().secondsSinceEpoch();
    bool earliestChanged = false;

    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    std::pair<TimerSet::iterator, bool> result = timers_.insert(std::make_pair(when, std::move(timer)));
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