#pragma once
#include <condition_variable>
#include <mutex>
#include "Thread.h"
#include <atomic>
#include <string>

class EventLoop;

class EventLoopThread
{
  public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

  private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    std::unique_ptr<Thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    static std::atomic_int s_numCreated_;
    std::string threadName_;
};
