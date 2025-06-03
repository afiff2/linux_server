#pragma once

#include "Callbacks.h"
#include "noncopyable.h"

#include <memory>
#include <mutex>
#include <string>

class EventLoop;
class InetAddress;

class TcpClient : noncopyable
{
  public:
    TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }

    EventLoop *getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    const std::string &name() const { return name_; }

    /// Set connection callback.
    /// Not thread safe.
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }

    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

  private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd);
    /// Not thread safe, but in loop
    void removeConnection(const TcpConnectionPtr &conn);

    EventLoop *loop_;
    ConnectorPtr connector_; // avoid revealing Connector
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    std::string name_;
    bool retry_;   // atomic
    bool connect_; // atomic
    // always in loop thread
    int nextConnId_;
    mutable std::mutex mutex_;
    TcpConnectionPtr connection_; // 访问前应当对mutex_加锁
};