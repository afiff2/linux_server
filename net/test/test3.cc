#include <sys/timerfd.h>
#include "EventLoop.h"
#include "Channel.h"
#include "Timestamp.h"

#include <cstdio>
#include <string.h>

EventLoop* g_loop;

void timeout(Timestamp receiveTime)
{
  printf("%s Timeout!\n", receiveTime.toFormattedString().c_str());
  g_loop->quit();
}

int main()
{
    printf("%s started\n", Timestamp::now().toFormattedString().c_str());
    EventLoop loop;
    g_loop = &loop;
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd,0,&howlong,NULL);

    loop.loop();
    ::close(timerfd);
    return 0;
}