#include <iostream>
#include <thread> 
#include <unistd.h>
#include "../../base/CurrentThread.h"
#include "../EventLoop.h"
#include "../../base/Logger.h"

EventLoop* g_loop;

void threadFunc()
{
    g_loop->loop();
}

int main()
{
    EventLoop loop;
    g_loop = &loop;

    std::thread t(threadFunc);

    t.join();

    return 0;
}