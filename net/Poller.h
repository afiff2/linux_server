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
    virtual ~Poller();

    // Polls the I/O events. Must be called in the loop thread
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    /// Must be called in the loop thread.
    virtual void updateChannel(Channel *channel) = 0;
    /// Must be called in the loop thread.
    virtual void removeChannel(Channel* channel) = 0;

    void assertInLoopThread() const;

    static Poller* newDefaultPoller(EventLoop* loop);
  
  protected:
    using ChannelMap = std::map<int, Channel *>;
    ChannelMap channels_;

  private:
    EventLoop *ownerLoop_;
};