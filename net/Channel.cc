#include "Channel.h"
#include "EventLoop.h"
#include <poll.h>
#include "Logger.h"
#include <cassert>

const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = POLLIN | POLLPRI;
const int Channel::KWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fdArg)
    : loop_(loop),
      fd_(fdArg),
      events_(0),
      revents_(0),
      index_(-1),
      eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent()
{
    //LOG_DEBUG << "Channel:handleEvent Start, eventHandling_:" << eventHandling_;
    eventHandling_ = true;
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "Channel::handle_enent() POLLNVAL";
    }
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) 
      {
        LOG_WARN << "Channel::handle_event() POLLHUP";
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (POLLERR|POLLNVAL))
    {
        if(errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if(readCallback_) readCallback_();
    }
    if (revents_ & POLLOUT)
    {
        if(writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
    //LOG_DEBUG << "Channel:handleEvent End, eventHandling_:" << eventHandling_;
}