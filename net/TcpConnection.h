#pragma once

#include "Callbacks.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include <memory>
#include <string>
#include <functional>

class Channel;
class EventLoop;
class Socket;
class Timestamp;
class Buffer;

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

    // called when TcpServer accepts a new connection
    void connectEstablished(); // should be called only once
    //Internal use only
    void setCloseCallback(const CloseCallback& cb)
    {closeCallback_ = cb;}
    // called when TcpServer accepts a new connection
    void connectDestroyed();// should be called only once

  private:
    enum StateE
    {
        KConnecting,
        KConnected,
        KDisconnected,
    };

    void setState(StateE s) { state_ = s; }
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

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
    Buffer inputBuffer_;
};