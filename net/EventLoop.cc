#include "EventLoop.h"
#include "Logger.h"
#include <cassert>
#include <poll.h>

thread_local EventLoop* t_loopInThisThread = nullptr;

//每个线程至多一个EventLoop
EventLoop::EventLoop() :  looping_(false), threadId_(CurrentThread::tid())
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread)
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

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
    LOG_FATAL << "ERROR: This method must be called in origin thread." ;
    abort();
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;

    poll(nullptr, 0, 5000);

    LOG_TRACE<< "EventLoop " << this << " stop looping";
    looping_ = false;
}