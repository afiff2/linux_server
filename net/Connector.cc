#include "Connector.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Logger.h"
#include <cassert>

const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

static int createNonblockingSocket()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_ERROR << "createNonblockingSocket() failed";
    }
    return sockfd;
}

static int getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = sizeof(optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno; // 返回错误码
    } else {
        return optval; // 返回 socket 错误状态
    }
}

static struct sockaddr_in getLocalAddr(int sockfd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (::getsockname(sockfd, (struct sockaddr*)&addr, &len) == -1) {
        bzero(&addr, sizeof(addr));
    }
    return addr;
}

static struct sockaddr_in getPeerAddr(int sockfd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (::getpeername(sockfd, (struct sockaddr*)&addr, &len) == -1) {
        bzero(&addr, sizeof(addr));
    }
    return addr;
}

static bool isSelfConnect(int sockfd) {
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port
        && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop), serverAddr_(serverAddr), connect_(false), state_(kDisconnected), retryDelayMs_(kInitRetryDelayMs), timerId_(TimerId::invalid())
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() { LOG_DEBUG << "dtor[" << this << "]"; }

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop([this]() { this->startInLoop(); });
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
        connect();
    else
        LOG_DEBUG << "do not connect";
}

void Connector::connect()
{
    int sockfd = createNonblockingSocket();
    const struct sockaddr_in *addr = serverAddr_.getSockAddr();

    int ret = ::connect(sockfd, reinterpret_cast<const struct sockaddr *>(addr), sizeof(*addr));
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_ERROR << "connect error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        break;

    default:
        LOG_ERROR << "Unexpected error in Connector::startInLoop " << savedErrno;
        ::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this](){this->handleWrite();}); // FIXME: unsafe
    channel_->setErrorCallback([this](){this->handleError();}); // FIXME: unsafe

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();
}

//避免重复调用handleWrite,handleError
int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    loop_->removeChannel(channel_.get());
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop([this](){this->resetChannel();}); // FIXME: unsafe
    return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << getErrnoMsg(err);
            retry(sockfd);
        }
        else if (isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError";
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << getErrnoMsg(err);
    retry(sockfd);
}

void Connector::retry(int sockfd)
{
    ::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort() << " in " << retryDelayMs_
                 << " milliseconds. ";
        timerId_ = loop_->runAfter(retryDelayMs_ / 1000.0, // FIXME: unsafe
                                    [this](){this->startInLoop();});
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}