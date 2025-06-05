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
    using BufferPtr   = std::unique_ptr<LargeBuffer>;
    using BufferVector = std::vector<BufferPtr>;

    void threadFunc();

    const int flushInterval_; // 日志刷新时间
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;

    // 这里拆成两个常量：前端备用缓冲池大小 和 后端备用缓冲池大小
    static const int frontBuffers = 2; // 前端除 current 外，最多保留 2 张备用
    static const int backBuffers  = 6; // 后端每次一轮想保留 6 张用于下一轮预分配

    // “前端备用”缓冲区，最多 frontBuffers 个
    BufferVector freeBuffers_;
    // “待写队列”：前端写满时把 full buffer push 到这里
    BufferVector buffers_;
};
