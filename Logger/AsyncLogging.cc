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

    // 前端备用池只保留 frontBuffers（2）张
    freeBuffers_.reserve(frontBuffers);
    for (int i = 0; i < frontBuffers; ++i)
    {
        BufferPtr buffer_temp(new LargeBuffer);
        buffer_temp->bzero();
        freeBuffers_.push_back(std::move(buffer_temp));
    }

    buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // currentBuffer_ 有足够空间
    if (currentBuffer_->avail() > static_cast<size_t>(len))
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        // currentBuffer_ 写满了
        buffers_.push_back(std::move(currentBuffer_));
        // 先尝试从前端备用 freeBuffers_ 拿一张
        if (!freeBuffers_.empty())
        {
            currentBuffer_ = std::move(freeBuffers_.back());
            freeBuffers_.pop_back();
        }
        else
        {
            // 没有备用时就 new 一张
            currentBuffer_.reset(new LargeBuffer);
        }
        currentBuffer_->bzero();
        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    LogFile output(basename_, rollSize_);

    // 后端“待写池”
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    // ——下面是后端每轮要预先分配的 backBuffers+1 张空缓冲
    BufferVector newbufferVec;
    newbufferVec.reserve(backBuffers + 1);
    for (int i = 0; i < backBuffers + 1; ++i)
    {
        BufferPtr buf(new LargeBuffer);
        buf->bzero();
        newbufferVec.push_back(std::move(buf));
    }

    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty())
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            // 无论超时还是被唤醒，都把 currentBuffer_ “强制” 移交给待写队列
            buffers_.push_back(std::move(currentBuffer_));

            // 1) 从 newbufferVec 弹出一张给前端新的 currentBuffer_
            currentBuffer_ = std::move(newbufferVec.back());
            newbufferVec.pop_back();

            // 2) 把 newbufferVec 里多余的缓冲补充给前端备用 freeBuffers_
            //    直到 freeBuffers_.size() == frontBuffers（2 张）
            while (freeBuffers_.size() < static_cast<size_t>(frontBuffers))
            {
                freeBuffers_.push_back(std::move(newbufferVec.back()));
                newbufferVec.pop_back();
            }

            // 3) 把 buffers_（前端写满的部分） swap 到 buffersToWrite
            buffersToWrite.swap(buffers_);
        }

        // 解锁后，实际把 buffersToWrite 里所有缓冲落盘
        for (const auto &buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        // 如果积压的缓冲超过 backBuffers+1（7 张），就丢弃多余的
        if (buffersToWrite.size() > static_cast<size_t>(backBuffers + 1))
        {
            buffersToWrite.resize(backBuffers + 1);
        }

        // 把用过的缓冲回收到 newbufferVec，保证 newbufferVec 恢复到 backBuffers+1 张
        while (newbufferVec.size() < static_cast<size_t>(backBuffers + 1))
        {
            newbufferVec.push_back(std::move(buffersToWrite.back()));
            buffersToWrite.pop_back();
            newbufferVec.back()->reset(); // 清空缓冲
        }
        buffersToWrite.clear();

        output.flush();
    }
    // 线程退出前再 flush 一次
    output.flush();
}
