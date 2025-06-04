#include "Poller.h"

#include "Channel.h"
#include "EventLoop.h"
#include "EPollPoller.h"
#include "PollPoller.h"
#include <unistd.h>

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

Poller::~Poller() = default;

void Poller::assertInLoopThread() const { ownerLoop_->assertInLoopThread(); }

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
#if defined(__linux__)
    return new EPollPoller(loop);
#else
    return new PollPoller(loop);
#endif
}