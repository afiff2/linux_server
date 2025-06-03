#include "TcpClient.h"

#include "Connector.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <cassert>

namespace detail
{
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop([conn]() { conn->connectDestroyed(); });
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

} // namespace detail

static EventLoop *CHECK_NOTNULL(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "main Loop is NULL!";
    }
    return loop;
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

TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg)
    : loop_(CHECK_NOTNULL(loop)), connector_(new Connector(loop, serverAddr)), name_(nameArg), retry_(false),
      connect_(true), nextConnId_(1)
{
    connector_->setNewConnectionCallback(
        [this](int sockfd){ this->newConnection(sockfd); }
    );
    // FIXME setConnectFailedCallback
    LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector " << connector_.get();
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector " << connector_.get();
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        // FIXME: not 100% safe, if we are in different thread
        loop_->runInLoop([this, conn]() {
            conn->setCloseCallback([this](const TcpConnectionPtr& connection) {
                detail::removeConnection(loop_, connection);
            });
        });
    }
    else
    {
        connector_->stop();
        // FIXME: HACK
        loop_->runAfter(1, [connector = connector_]() {
            detail::removeConnector(connector);
        });
    }
}

void TcpClient::connect()
{
    // FIXME: check state
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to " << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    InetAddress localAddr(getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        [this](const TcpConnectionPtr &conn){ this->removeConnection(conn); }
    ); // FIXME: unsafe

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to " << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}
