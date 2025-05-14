#include "Logger.h"
#include <ctime>

Logger::LogLevel Logger::g_logLevel = LogLevel::INFO;

// 实现你缺失的两个构造函数
Logger::Logger(SourceFile file, int line) {
    std::cerr << file.data_ << ":" << line << " " ;
}

Logger::Logger(SourceFile file, int line, LogLevel level) {
    std::cerr << file.data_ << ":" << line << " [" << static_cast<int>(level) << "] ";
}

// 析构函数也要有
Logger::~Logger() {
    std::cerr << std::endl;
}