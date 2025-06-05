#include <string>
#include <iostream>
#include <sys/stat.h>
#include <sstream>

#include "TcpConnection.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Logger.h"
#include "AsyncLogging.h"

static const off_t kRollSize = 1 * 1024 * 1024; // 1MB

AsyncLogging* g_asyncLog = nullptr;

// 全局回调供 Logger 使用
void asyncLog(const char* msg, int len)
{
    if (g_asyncLog)
    {
        g_asyncLog->append(msg, len);
    }
}

int main(int argc, char* argv[])
{
    // —— 1. 启动异步日志模块 —— 
    const std::string LogDir = "logs";
    mkdir(LogDir.c_str(), 0755);

    // 日志文件名前缀：logs/可执行文件名
    std::ostringstream oss;
    oss << LogDir << "/" << ::basename(argv[0]);
    AsyncLogging log(oss.str(), kRollSize);
    g_asyncLog = &log;

    // 把 Logger 重定向到 asyncLog
    Logger::setOutput(asyncLog);
    log.start();

    // —— 2. 构造 TcpServer —— 
    EventLoop loop;
    InetAddress listenAddr(8080);
    TcpServer server(&loop, listenAddr);
    server.setThreadNum(3);

    // —— 3. 注册回调 —— 
    // 3.1 连接建立/断开回调
    server.setConnectionCallback(
        [](const TcpConnectionPtr& conn) {
            if (conn->connected())
            {
                LOG_INFO << "Connection UP   : "
                         << conn->peerAddress().toIpPort().c_str();
            }
            else
            {
                LOG_INFO << "Connection DOWN : "
                         << conn->peerAddress().toIpPort().c_str();
            }
        }
    );

    // 3.2 消息可读回调：Echo + 如果收到 "shutdown\n" 就退出
    server.setMessageCallback(
        // 捕获 loop 的引用，以便在检测到关键消息时调用 loop.quit()
        [&loop](const TcpConnectionPtr& conn, Buffer* buf, Timestamp tm) {
            std::string msg = buf->retrieveAllAsString();
            // 把收到的内容回显给客户端
            conn->send(msg);

            // 如果客户端发来 "shutdown\n"，触发服务器关闭
            if (msg == "shutdown\n" || msg == "shutdown")
            {
                LOG_WARN << "Received shutdown command from "
                         << conn->peerAddress().toIpPort().c_str()
                         << ", server will quit.";
                // 通知 EventLoop 退出
                loop.quit();
            }
            else
            {
                LOG_INFO << "Echoed " << msg.size()
                         << " bytes at " << tm.toString().c_str();
            }
        }
    );

    // —— 4. 启动服务器 —— 
    server.start();
    std::cout << "===== Web Server Started =====\n";

    //    这行会一直阻塞直到 loop.quit() 被调用
    loop.loop();
    std::cout << "===== Web Server Stopped =====\n";

    // —— 5. 停止异步日志 —— 
    log.stop();
    return 0;
}
