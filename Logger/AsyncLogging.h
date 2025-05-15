#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogFile.h"

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

class AsyncLogging : noncopyable
{
public:
    // flushInterval：日志刷新时间，rollSize：日志文件最大尺寸
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }
    // 前端调用 append 写入日志
    void append(const char *logline, int len);
    void start()
    {
        running_ = true;
        thread_.start();
    }
    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    using LargeBuffer = FixedBuffer<kLargeBufferSize>;
    using BufferVector = std::vector<std::unique_ptr<LargeBuffer>>;
    using BufferPtr = std::unique_ptr<LargeBuffer>;
    void threadFunc();
    
    const int flushInterval_;         // 日志刷新时间
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    
    BufferPtr currentBuffer_;
    // 扩充前端缓冲区数量到8个
    // currentBuffer_ + freeBuffers_ 中的缓冲区总数就是8个
    BufferVector freeBuffers_;
    BufferVector buffers_;  // 待写入后端线程的已满缓冲区
    
    static const int maxFreeBuffers = 7; // 备用缓冲区上限（不包括 currentBuffer_）
};
