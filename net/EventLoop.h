#pragma once

#include "noncopyable.h"
#include "CurrentThread.h"

#include <unistd.h>
#include <sys/syscall.h>


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
    
    private:
        void abortNotInLoopThread();

        bool looping_;// atomic
        const pid_t threadId_;
};