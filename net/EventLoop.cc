#include "EventLoop.h"
#include <cassert>
#include <cstdlib>
#include <sys/eventfd.h>
#include <functional>

#include "Logger.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Timestamp.h"

thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 5000;

static int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

// 每个线程至多一个EventLoop
EventLoop::EventLoop()
    : looping_(false), threadId_(CurrentThread::tid()), poller_(new Poller(this)), timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
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
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    assert(!looping_);
    ::close(wakeupFd_);
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
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
        {
            (*it)->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    // 跨线程：需要 wakeup() 打断 poll()，才能让循环意识到 quit_ == true 并退出。
    if (!isInLoopThread())
        wakeup();
}

TimerId EventLoop::runAt(const Timestamp &time, const Timer::TimerCallback &cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const Timer::TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const Timer::TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::runInLoop(const Functor &cb)
{
    if (isInLoopThread())
        cb();
    else
        queueInLoop(cb);
}

void EventLoop::queueInLoop(const Functor &cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    // 如果doPendingFunctors正在被调用，他调用的Functor可能又调用了queueInLoop，必需wakeup，否则新的cb不能被即时调用
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const auto &func : functors)
        func();
    callingPendingFunctors_ = false;
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}