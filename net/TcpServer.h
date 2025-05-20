#pragma oncce
#include "noncopyable.h"
#include "Callbacks.h"
#include "map"
#include <string>
#include <memory>

class EventLoop;
class InetAddress;
class Acceptor;


class TcpServer : noncopyable
{
    public:
        TcpServer(EventLoop* loop, const InetAddress& listenAddr);
        ~TcpServer();

        //thread safe
        void start();
        //Not thread safe
        void setConnectionCallback(const ConnectionCallback& cb)
        {connectionCallback_ = cb;}

        //Not thread safe
        void setMessageCallback(const MessageCallback& cb)
        {messagecallback_ = cb;}

    private:
        //Not thread safe but in loop
        void newConnection(int sockfd, const InetAddress& peerAddr);

        void removeConnection(const TcpConnectionPtr& conn);

        using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
        
        EventLoop* loop_;
        const std::string name_;
        std::unique_ptr <Acceptor> acceptor_;
        ConnectionCallback connectionCallback_;
        MessageCallback messagecallback_;
        bool started_;
        int nextConnId_;//always in loop thread;
        ConnectionMap connections_;
};