#include "TcpServer.h"

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include <cassert>

static EventLoop *CHECK_NOTNULL(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "main Loop is NULL!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(CHECK_NOTNULL(loop)), name_(listenAddr.toIpPort()), acceptor_(new Acceptor(loop, listenAddr)),
      started_(false), nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress &peerAddr){
            this->newConnection(sockfd, peerAddr);
        }
    );
}

TcpServer::~TcpServer()
{

}

void TcpServer::start()
{
    if(!started_)
        started_ = true;
    
    if(!acceptor_->listenning())
        loop_->runInLoop(
            [this](){
                acceptor_->listen();
            }
        );
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from "
             << peerAddr.toIpPort();

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr";
    }

    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setHighWaterMarkCallback(highWaterMarkCallback_, highWaterMark_);
    //不能用值传递conn，否则conn和lamdba相互引用，不会被析构
    conn->setCloseCallback(
        [this](const TcpConnectionPtr& conn) {
            this->removeConnection(conn);
        }
    );
    conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnection [" << name_
        << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    //非常重要，使用queueInLoop让conn->connectDestroyed()在稍后被执行，否则conn被析构，会释放内部成员channel，但是此时channel正在执行TcpServer::removeConnection
    loop_->queueInLoop(
        [conn](){
            conn->connectDestroyed();
        }
    );
    //LOG_TRACE << "TcpServer::removeConnection End";
}