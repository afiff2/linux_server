#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sstream>

#include "AsyncLogging.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "Thread.h"
#include "Timestamp.h"

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

// 并发线程数
static const int kThreadCount = 20;

// 每个客户端线程要发送的最小和最大消息数
static const int kMinMessages = 10;
static const int kMaxMessages = 100;

// 服务器地址
static const int kServerPort = 8080;
static const char *kServerIp = "127.0.0.1";

// 线程函数：每个线程启动一个 EventLoop + TcpClient
// 随机决定向服务器发送 [kMinMessages, kMaxMessages] 条消息，然后断开
void clientThreadFunc(int threadNum)
{
    // 生成随机种子,时间加序列号
    std::mt19937 rng(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) +
                     threadNum);
    std::uniform_int_distribution<int> dist(kMinMessages, kMaxMessages);
    int messagesToSend = dist(rng);

    EventLoop loop;
    InetAddress serverAddr(kServerPort, kServerIp);

    // 命名 "Client-<线程号>"
    std::ostringstream nameOs;
    nameOs << "Client-" << threadNum;
    std::string clientName = nameOs.str();

    TcpClient client(&loop, serverAddr, clientName);
    client.enableRetry(); // 如果连接失败，自动重连

    // 记录已经发送和收到的消息数
    std::shared_ptr<int> sentCount(new int(0));
    std::shared_ptr<int> recvCount(new int(0));

    // 连接建立/断开时的回调
    client.setConnectionCallback(
        [clientName, sentCount, &loop](const TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                LOG_INFO << clientName << " - Connected to server: " << conn->peerAddress().toIpPort().c_str();
                // 立刻发送第一条消息
                std::string msg = "Hello from " + clientName + " (" + std::to_string(*sentCount + 1) + ")\n";
                conn->send(msg);
                (*sentCount)++;
            }
            else
            {
                LOG_INFO << clientName << " - Disconnected from server";
                // 当连接断开后，结束 EventLoop，线程将退出
                loop.quit();
            }
        });

    // 收到服务器回显消息时的回调
    client.setMessageCallback(
        [clientName, sentCount, recvCount, &messagesToSend, &client, &loop](const TcpConnectionPtr &conn, Buffer *buf,
                                                                            Timestamp recvTime)
        {
            std::string echo = buf->retrieveAllAsString();
            (*recvCount)++;
            LOG_INFO << clientName << " - Received echo [" << echo.substr(0, echo.size() - 1) << "] " << *recvCount
                     << "/" << *sentCount << " at " << recvTime.toFormattedString().c_str();

            // 如果还没达到要发送的总数，就发下一条
            if (*sentCount < messagesToSend)
            {
                std::string msg = "Hello from " + clientName + " (" + std::to_string(*sentCount + 1) + ")\n";
                conn->send(msg);
                (*sentCount)++;
            }
            else
                conn->shutdown();
        });

    client.connect();
    loop.loop();
    LOG_INFO << clientName << " - Thread exiting after sending " << *sentCount << " messages and receiving "
             << *recvCount;
}

int main(int argc, char *argv[])
{
    // —— 1. 启动异步日志 ——
    const std::string logDir = "logs";
    mkdir(logDir.c_str(), 0755);

    std::ostringstream oss;
    oss << logDir << "/" << ::basename(argv[0]);
    AsyncLogging log(oss.str(), kRollSize);
    g_asyncLog = &log;
    Logger::setOutput(asyncLog);
    log.start();

    LOG_INFO << "Client starting stress test";

    // —— 2. 启动多个客户端线程做压力测试 ——
    std::vector<std::unique_ptr<Thread>> threads;
    threads.reserve(kThreadCount);

    for (int i = 0; i < kThreadCount; ++i)
    {
        // 每个线程传入一个线程编号
        auto func = std::bind(clientThreadFunc, i + 1);
        threads.emplace_back(new Thread(func, "ClientThread"));
        threads.back()->start();
    }

    // 等待所有线程结束
    for (auto &thr : threads)
    {
        thr->join();
    }

    LOG_INFO << "All client threads finished";

    // —— 3. 所有线程发送完成后，向服务器发送 "shutdown" ——
    {
        EventLoop loop;
        InetAddress serverAddr(kServerPort, kServerIp);
        TcpClient shutdownClient(&loop, serverAddr, "ShutdownClient");

        // 在连接建立后立刻发送 "shutdown\n"
        shutdownClient.setConnectionCallback(
            [&loop](const TcpConnectionPtr &conn)
            {
                if (conn->connected())
                {
                    LOG_WARN << "Sending shutdown command to server";
                    conn->send("shutdown\n");
                    conn->shutdown();
                }
                else
                {
                    // 服务器已断开，退出循环
                    loop.quit();
                }
            });
        // 需要一个空的 onMessage 回调，避免 bad_function_call
        shutdownClient.setMessageCallback(
            [](const TcpConnectionPtr&, Buffer*, Timestamp) {
            }
        );
        shutdownClient.enableRetry();
        shutdownClient.connect();
        loop.loop();
        LOG_INFO << "Shutdown command sent, exiting client";
    }

    // —— 4. 停止异步日志 ——
    log.stop();
    return 0;
}