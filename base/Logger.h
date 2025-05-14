#pragma once

#include <iostream>
#include <string>

class Logger {
public:
    enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

    // SourceFile 简化版
    class SourceFile {
    public:
        template<int N>
        SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {}

        const char* data_;
        int size_;
    };

    // 构造函数 —— 这个是你现在缺少的！
    Logger(SourceFile file, int line);

    // 另一个可能的日志等级构造函数
    Logger(SourceFile file, int line, LogLevel level);

    // 析构函数
    ~Logger();

    // 返回日志流，支持链式调用
    class LogStream {
    public:
        template<typename T>
        LogStream& operator<<(T const& value) {
            std::cerr << value;
            return *this;
        }
    };

    LogStream& stream() { return stream_; }

private:
    LogStream stream_;
    static LogLevel g_logLevel;
};

extern "C" void abortNotInLoopThread();


#define LOG_INFO Logger(__FILE__, __LINE__).stream()
#define LOG_TRACE Logger(__FILE__, __LINE__).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__).stream()