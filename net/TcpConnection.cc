#include "TcpConnection.h"

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "Timestamp.h"

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
    channel_->setReadCallback([this](Timestamp receiveTime) { this->handleRead(receiveTime); });
    channel_->setWriteCallback([this]() { this->handleWrite(); });
    channel_->setCloseCallback([this]() { this->handleClose(); });
    channel_->setErrorCallback([this]() { this->handleError(); });
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == KConnecting);
    setState(KConnected);
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    else if (n == 0)
        handleClose();
    else
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpConnection::handleClose state = " << state_;
    assert(state_ == KConnected || state_ == KDisconnecting);
    // 析构关闭fd，方便定位没有析构的TcpConnection
    channel_->disableAll();
    // 回调要放在最后，否则channel_可能已经被销毁
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError name:" << name_.c_str() << "- SO_ERROR:%" << err;
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    assert(state_ == KConnected || state_ == KDisconnecting);
    setState(KDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());

    loop_->removeChannel(channel_.get());
}

void TcpConnection::shutdown()
{
    if (state_ == KConnected)
    {
        setState(KDisconnecting);
        loop_->runInLoop([this]() { this->shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
        socket_->shutdownWrite();
}

void TcpConnection::send(std::string message)
{
    if (state_ == KConnected)
    {
        if (loop_->isInLoopThread())
            sendInLoop(std::move(message));
        else
            loop_->runInLoop([this, msg = std::move(message)]() { this->sendInLoop(std::move(msg)); });
    }
}

void TcpConnection::sendInLoop(const std::string &message)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0)
        {
            if (static_cast<size_t>(nwrote) < message.size())
                LOG_TRACE << "I am going to write more data";
            else if (writeCompleteCallback_)
                loop_->queueInLoop([weakSelf = shared_from_this()]() { weakSelf->writeCompleteCallback_(weakSelf); });
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
            }
        }
    }

    assert(nwrote >= 0);
    if (static_cast<size_t>(nwrote) < message.size())
    {
        size_t remaining = message.size() - nwrote;
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
            loop_->queueInLoop([weakSelf = shared_from_this(), oldLen, remaining]() { weakSelf->highWaterMarkCallback_(weakSelf,  oldLen + remaining);});
        outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
        if (!channel_->isWriting())
            channel_->enableWriting(); // level-triggered , 只要 socket 可写，就会持续触发写事件
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting(); // 防止一直发
                if (writeCompleteCallback_)
                    loop_->queueInLoop([weakSelf = shared_from_this()]()
                                       { weakSelf->writeCompleteCallback_(weakSelf); });
                if (state_ == KDisconnecting)
                {
                    shutdownInLoop();
                }
            }
            else
            {
                LOG_TRACE << "I am going to write more data";
            }
        }
        else
        {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG_TRACE << "Connection is down, no more writing";
    }
}

void TcpConnection::setTcpNoDelay(bool on) { socket_->setTcpNoDelay(on); }

void TcpConnection::setKeepAlive(bool on) { socket_->setKeepAlive(on); }