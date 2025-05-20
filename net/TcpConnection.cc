#include "TcpConnection.h"

#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logger.h"

#include <cassert>

static EventLoop *CHECK_NOTNULL(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "main Loop is NULL!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd, const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(nameArg), state_(KConnecting), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr)
{
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd=" << sockfd;
    channel_->setReadCallback([this](){
        this->handleRead();
    });
}

TcpConnection::~TcpConnection()
{
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == KConnecting);
    setState(KConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead()
{
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    messageCallback_(shared_from_this(), buf, n);
}