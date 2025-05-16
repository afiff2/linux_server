#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Timer.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <vector>
#include <Channel.h>
#include <memory>

class Poller;
class TimerId;
class Timestamp;
class TimerQueue;

class EventLoop : noncopyable
{
    public:
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


    
    private:
        void abortNotInLoopThread();


        using ChannelList = std::vector<Channel*>;
        

        bool looping_;// atomic
        bool quit_;// atomic
        const pid_t threadId_;
        std::unique_ptr<Poller> poller_;
        ChannelList activeChannels_;
        std::unique_ptr<TimerQueue> timerQueue_;
};