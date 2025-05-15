#include <iostream>
#include <thread> 
#include <unistd.h>
#include "CurrentThread.h"
#include "EventLoop.h"
#include "Logger.h"

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop loop;
    loop.loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

    EventLoop loop;
    std::thread t(threadFunc);

    loop.loop();

    t.join();

    return 0;
}