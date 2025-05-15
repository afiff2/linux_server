#pragma once

#include <functional>

#include "noncopyable.h"

class EventLoop;

//对 I/O 事件的封装 ，通常是对文件描述符（fd）及其感兴趣的事件（读、写等）的封装
//在io线程中调用，不必加锁
class Channel : noncopyable
{
    public:
        using EventCallback = std::function<void()>;
        Channel(EventLoop* loop,int fd);

        void handleEvent();
        void setReadCallback(const EventCallback& cb) {readCallback_ = cb;}
        void setWriteCallback(const EventCallback& cb) {writeCallback_ = cb;}
        void setErrorCallback(const EventCallback& cb) {errorCallback_ = cb;}

        int fd() const {return fd_;}
        int events() const {return events_;}
        void set_revents_(int revt) {revents_ = revt;}
        bool isNoneEvent() const {return events_ == KNoneEvent;}

        void enableReading() {events_ |= KReadEvent; update();}
        void enableWriting() {events_ |= KWriteEvent; update();}
        void disableReading() {events_ &= ~KWriteEvent; update();}
        void disableAll() {events_ |= KNoneEvent; update();}

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

        EventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback errorCallback_;

};