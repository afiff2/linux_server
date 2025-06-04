#pragma once
#include "Poller.h"

#include <vector>

class Channel;
class EventLoop;

struct pollfd;

class PollPoller : public Poller
{
  public:
    PollPoller(EventLoop *loop);
    ~PollPoller() override;

    // Polls the I/O events. Must be called in the loop thread
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    /// Must be called in the loop thread.
    void updateChannel(Channel *channel) override;
    /// Must be called in the loop thread.
    void removeChannel(Channel* channel) override;

  private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    using PollFdList = std::vector<struct pollfd>;
    PollFdList pollfds_;
};