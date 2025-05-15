#pragma once
#include <string.h>
#include <string>

constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

// 固定长度的缓冲区
template <int buffer_size>
class FixedBuffer : noncopyable
{
public:
    FixedBuffer()
        : cur_(data_), size_(0)
    {
    }

    void append(const char *buf, size_t len)
    {
        if (avail() > len)
        {
            memcpy(cur_, buf, len);
            add(len);
        }
    }

    const char *data() const { return data_; }

    int length() const { return size_; }

    // 返回当前指针的位置，开放给外界修改
    char *current() { return cur_; }

    // 返回剩余可用空间的大小
    size_t avail() const { return static_cast<size_t>(buffer_size - size_); }

    // 更新当前指针，增加指定长度
    void add(size_t len)
    {
        cur_ += len;
        size_ += len;
    }
    // 重置当前指针
    void reset()
    {
        cur_ = data_;
        size_ = 0;
    }

    // 清空缓冲区的数据
    void bzero() { ::bzero(data_, sizeof(data_)); }

    // 将缓冲区中的数据转换为std::string类型并返回
    std::string toString() const { return std::string(data_, length()); }

private:
    char data_[buffer_size];
    char *cur_;              // 指向下一个可写入的位置
    int size_;               // 长度
};
