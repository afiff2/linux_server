#include "EventLoopThreadPool.h"

#include "EventLoop.h"

#include "cassert"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop) : baseloop_(baseLoop), started_(false), numThreads_(0), next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start()
{
    assert(!started_);
    baseloop_->assertInLoopThread();
    started_ = true;

    for(int i=0;i<numThreads_;++i)
    {
        threads_.emplace_back(std::make_unique<EventLoopThread>());
        loops_.push_back(threads_.back()->startLoop());
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseloop_->assertInLoopThread();
    EventLoop* loop = baseloop_;
    if(!loops_.empty())
    {
        //round-robiin
        loop = loops_[next_];
        ++next_;
        if(static_cast<size_t>(next_)>=loops_.size())
            next_= 0;
    }
    return loop;
}