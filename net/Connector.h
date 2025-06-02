#pragma once

#include "noncopyable.h"
#include "TimerId.h"
#include "InetAddress.h"

#include <functional>
#include <memory>

class EventLoop;
class Channel;

class Connector : noncopyable
{
    public:
        using NewConnectionCallback = std::function<void (int sockfd)>;

        Connector(EventLoop* loop, const InetAddress& serverAddr);
        ~Connector();

        void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb;}

        void start(); // any thread
        void restart(); // must be called in loop thread
        void stop(); // any thread

    private:
        enum States { kDisconnected, kConnecting, kConnected };
        static const int kMaxRetryDelayMs = 30*1000;
        static const int kInitRetryDelayMs = 500;

        void setState(States s) { state_ = s; }
        void startInLoop();
        void connect();
        void connecting(int sockfd);
        void handleWrite();
        void handleError();
        void retry(int sockfd);
        int removeAndResetChannel();
        void resetChannel();

        EventLoop* loop_;
        InetAddress serverAddr_;
        bool connect_; // atomic
        States state_;  // FIXME: use atomic variable
        std::shared_ptr<Channel> channel_;
        NewConnectionCallback newConnectionCallback_;
        int retryDelayMs_;
        TimerId timerId_;
};