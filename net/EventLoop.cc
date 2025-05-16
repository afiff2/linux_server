#include "EventLoop.h"
#include "Logger.h"
#include <cassert>

#include "Poller.h"
#include "Timestamp.h"
#include "TimerId.h"
#include "TimerQueue.h"

thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 5000;

// 每个线程至多一个EventLoop
EventLoop::EventLoop() : looping_(false), threadId_(CurrentThread::tid()), poller_(new Poller(this)), timerQueue_(new TimerQueue(this))
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << "exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    t_loopInThisThread = NULL;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread() { return t_loopInThisThread; }

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "ERROR: This method must be called in origin thread.";
    abort();
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
        {
            (*it)->handleEvent();
        }
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    //wakeup();
}

TimerId EventLoop::runAt(const Timestamp& time, const Timer::TimerCallback& cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const Timer::TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const Timer::TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}