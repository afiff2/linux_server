#include "EventLoop.h"
#include "Logger.h"
#include <cassert>

#include "Poller.h"

thread_local EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 5000;

// 每个线程至多一个EventLoop
EventLoop::EventLoop() : looping_(false), threadId_(CurrentThread::tid()), poller_(new Poller(this))
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

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}