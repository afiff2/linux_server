#pragma once
#include "Poller.h"

#include <vector>

class Channel;
class EventLoop;

struct epoll_event;

class EPollPoller : public Poller
{
  public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // Polls the I/O events. Must be called in the loop thread
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    /// Must be called in the loop thread.
    void updateChannel(Channel *channel) override;
    /// Must be called in the loop thread.
    void removeChannel(Channel* channel) override;

  private:
    void update(int operation, Channel* channel);

    static const char* operationToString(int op);
    
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    using EventList = std::vector<struct epoll_event>;
    EventList events_;
    int epollfd_;
};