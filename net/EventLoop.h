#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Timer.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <vector>
#include <Channel.h>
#include <memory>
#include <mutex>

#include "TimerId.h"

class Poller;
class Timestamp;
class TimerQueue;

class EventLoop : noncopyable
{
    public:
        using Functor = std::function<void()>;

        EventLoop();
        ~EventLoop();

        void loop();

        //判断当前线程是不是创建EventLoop的线程
        void assertInLoopThread()
        {
            if(!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        }

        bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

        EventLoop* getEventLoopOfCurrentThread();

        void updateChannel(Channel* channel);

        void quit();

        TimerId runAt(const Timestamp& time, const Timer::TimerCallback& cb);
        TimerId runAfter(double delay, const Timer::TimerCallback& cb);
        TimerId runEvery(double interval, const Timer::TimerCallback& cb);

        void runInLoop(const Functor& cb);
        void queueInLoop(const Functor& cb);


    
    private:
        using ChannelList = std::vector<Channel*>;

        void abortNotInLoopThread();
        void wakeup();
        void handleRead();//wake up
        void doPendingFunctors();




        

        bool looping_;// atomic
        bool quit_;// atomic
        bool callingPendingFunctors_;//atomic
        const pid_t threadId_;
        std::unique_ptr<Poller> poller_;
        Timestamp pollReturnTime_;
        ChannelList activeChannels_;
        std::unique_ptr<TimerQueue> timerQueue_;
        int wakeupFd_;
        std::unique_ptr<Channel> wakeupChannel_;
        std::mutex mutex_;
        std::vector<Functor> pendingFunctors_; //暴露给线程，需要mutex保护
};