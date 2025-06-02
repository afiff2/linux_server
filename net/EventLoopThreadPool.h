#pragma once

#include <memory>
#include <vector>

#include "noncopyable.h"
#include "EventLoopThread.h"

class EventLoop;

class EventLoopThreadPool : noncopyable
{
    public:
        EventLoopThreadPool(EventLoop* baseLoop);
        ~EventLoopThreadPool();

        void setThreadNum(int numThreads) { numThreads_ = numThreads;}
        void start();
        EventLoop* getNextLoop();

    private:
        EventLoop* baseloop_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
};