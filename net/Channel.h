#pragma once

#include <functional>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;

//对 I/O 事件的封装 ，通常是对文件描述符（fd）及其感兴趣的事件（读、写等）的封装
//在io线程中调用，不必加锁
class Channel : noncopyable
{
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(Timestamp)>;
        Channel(EventLoop* loop,int fd);
        ~Channel();

        void handleEvent(Timestamp receiveTime);
        void setReadCallback(const ReadEventCallback& cb) {readCallback_ = cb;}
        void setWriteCallback(const EventCallback& cb) {writeCallback_ = cb;}
        void setErrorCallback(const EventCallback& cb) {errorCallback_ = cb;}
        void setCloseCallback(const EventCallback& cb) {closeCallback_ = cb;}


        int fd() const {return fd_;}
        int events() const {return events_;}
        void set_revents_(int revt) {revents_ = revt;}
        bool isNoneEvent() const {return events_ == KNoneEvent;}

        void enableReading() {events_ |= KReadEvent; update();}
        void enableWriting() {events_ |= KWriteEvent; update();}
        void disableWriting() {events_ &= ~KWriteEvent; update();}
        void disableAll() {events_ = KNoneEvent; update();}
        bool isWriting() const {return events_ & KWriteEvent;}

        //for Poller
        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        EventLoop* ownerLoop() { return loop_;}


    private:
        void update();

        static const int KNoneEvent;
        static const int KReadEvent;
        static const int KWriteEvent;

        EventLoop* loop_;
        const int fd_;
        int events_;
        int revents_;
        int index_; //该Channel对应pollfdlist_中的第index_个pollfd
        bool eventHandling_;

        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback errorCallback_;
        EventCallback closeCallback_;
};