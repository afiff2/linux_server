#include "AsyncLogging.h"
#include <stdio.h>

AsyncLogging::AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      currentBuffer_(new LargeBuffer)
{
    currentBuffer_->bzero();
    // 分配备用缓冲区
    freeBuffers_.reserve(maxFreeBuffers);
    for(int i=0;i<maxFreeBuffers;++i)
    {
        BufferPtr buffer_temp(new LargeBuffer);
        buffer_temp->bzero();
        freeBuffers_.push_back(std::move(buffer_temp));
    }
    buffers_.reserve(16); // 保证 buffers_ 有足够的空间
}

void AsyncLogging::append(const char *logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 如果当前缓冲区剩余空间足够，直接写入
    if (currentBuffer_->avail() > static_cast<size_t>(len))
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));
        if (!freeBuffers_.empty())
        {
            currentBuffer_ = std::move(freeBuffers_.back());
            freeBuffers_.pop_back();
        }
        else
        {
            currentBuffer_.reset(new LargeBuffer);
        }
        currentBuffer_->append(logline, len);
        // 通知后端线程
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    LogFile output(basename_, rollSize_);
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    BufferVector newbufferVec;
    
    //额外多一个给currentBuffer_
    newbufferVec.reserve(maxFreeBuffers+1);
    for(int i=0;i<maxFreeBuffers+1;++i)
    {
        BufferPtr buffer_temp(new LargeBuffer);
        buffer_temp->bzero();
        newbufferVec.push_back(std::move(buffer_temp));
    }
    
    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                // 等待一定的刷新间隔或有数据到来
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            // 将当前缓冲区中的数据也纳入待写队列
            buffers_.push_back(std::move(currentBuffer_));
            // 尝试从备用缓冲区中获取新的 currentBuffer_
            currentBuffer_ = std::move(newbufferVec.back());
            newbufferVec.pop_back();

            while(freeBuffers_.size()<maxFreeBuffers)
            {
                freeBuffers_.push_back(std::move(newbufferVec.back()));
                newbufferVec.pop_back();
            }

            buffersToWrite.swap(buffers_);
        }
        
        // 将所有待写缓冲区中的日志写入文件
        for (const auto &buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > maxFreeBuffers+1)
        {
            buffersToWrite.resize(maxFreeBuffers+1);
        }
        while(newbufferVec.size()<maxFreeBuffers+1)
        {
            newbufferVec.push_back(std::move(buffersToWrite.back()));
            buffersToWrite.pop_back();
            newbufferVec.back()->reset();
        }
        buffersToWrite.clear(); // 清空后端缓冲队列
        output.flush();
    }
    output.flush();
}
