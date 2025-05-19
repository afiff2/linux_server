#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

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
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
