#pragma once
#include "Timestamp.h"
#include "noncopyable.h"

#include <map>
#include <vector>

class Channel;
class EventLoop;

struct pollfd;

class Poller : noncopyable
{
  public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    ~Poller();

    // Polls the I/O events. Must be called in the loop thread
    Timestamp poll(int timeoutMs, ChannelList *activeChannels);
    /// Must be called in the loop thread.
    void updateChannel(Channel *channel);
    /// Must be called in the loop thread.
    void removeChannel(Channel* channel);

    void assertInLoopThread() const;

  private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    using PollFdList = std::vector<struct pollfd>;
    using ChannelMap = std::map<int, Channel *>;

    EventLoop *ownerLoop_;
    PollFdList pollfds_;
    ChannelMap channels_;
};