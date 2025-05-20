#include "TcpServer.h"

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "Acceptor.h"


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
    connetions_[connName] = conn;
    conn->setConnectionCallback(connetionCallback_);
    conn->setMessageCallback(messagecallback_);
    conn->connectEstablished();
}