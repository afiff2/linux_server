#include "EventLoopThread.h"
#include "EventLoop.h"
#include <cassert>

std::atomic_int EventLoopThread::s_numCreated_{ 0 };

EventLoopThread::EventLoopThread() : loop_(nullptr), exiting_(false), thread_(nullptr)
{
    int count = ++s_numCreated_;
    threadName_ = "EventLoopThread" + std::to_string(count);
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_)
        loop_->quit();
    if (thread_ && thread_->started())
        thread_->join();
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.reset(new Thread(
        std::bind(&EventLoopThread::threadFunc, this),
        std::string("EventLoopThread")
    ));
    thread_->start();

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return loop_ != nullptr; });
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
}