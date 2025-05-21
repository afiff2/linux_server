#pragma once

#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "Buffer.h"
#include <memory>
#include <string>
#include <functional>

class Channel;
class EventLoop;
class Socket;
class Timestamp;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
  public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() { return localAddr_; }
    const InetAddress &peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == KConnected; }

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {writeCompleteCallback_ = cb;}

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    // called when TcpServer accepts a new connection
    void connectEstablished(); // should be called only once
    //Internal use only
    void setCloseCallback(const CloseCallback& cb)
    {closeCallback_ = cb;}
    // called when TcpServer accepts a new connection
    void connectDestroyed();// should be called only once

    //建议输入右值引用
    void send(std::string message);
    //Thread safe
    void shutdown();

    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);

  private:
    enum StateE
    {
      KConnecting,
      KConnected,
      KDisconnecting,
      KDisconnected,
    };

    void setState(StateE s) { state_ = s; }
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const std::string& message);
    void shutdownInLoop();

    EventLoop *loop_;
    std::string name_;
    StateE state_; // use atomic variable
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
    WriteCompleteCallback writeCompleteCallback_;//低水位回调
    HighWaterMarkCallback highWaterMarkCallback_;//高水位回调
    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};