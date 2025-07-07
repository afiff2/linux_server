#include <chrono>
#include <iostream>
#include <memory>
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

// 每个线程在一次连接里要发的消息数
// 建议设置成较大的数字，比如 50k、100k，以制造足够压力
static const int kMessagesPerThread = 1000000;

// 服务器地址
static const int kServerPort = 8080;
static const char *kServerIp = "127.0.0.1";

void clientThreadFunc(int threadNum)
{
    EventLoop loop;
    InetAddress serverAddr(kServerPort, kServerIp);

    // 命名 "Client-<线程号>"
    std::ostringstream nameOs;
    nameOs << "Client-" << threadNum;
    std::string clientName = nameOs.str();

    TcpClient client(&loop, serverAddr, clientName);
    client.enableRetry(); // 如果连接失败，自动重连
    
    client.setConnectionCallback(
        [&client, clientName, &loop](const TcpConnectionPtr& conn) {
            if (conn->connected())
            {
                LOG_INFO << clientName << " - Connected to server: " << conn->peerAddress().toIpPort().c_str();

                std::string base = "Hello from " + clientName + " #";
                for (int i = 1; i <= kMessagesPerThread; ++i)
                {
                    std::string msg = base + std::to_string(i) + "\n";
                    conn->send(msg);
                }

                client.disconnect();
            }
            else
            {
                LOG_INFO << clientName << " - Disconnected from server";
                loop.quit();
            }
        }
    );

    std::shared_ptr<int> recvCount(new int(0));

    client.setMessageCallback(
        [clientName, recvCount](const TcpConnectionPtr &conn, Buffer *buf, Timestamp recvTime)
        {
            std::string echo = buf->retrieveAllAsString();
            ++(*recvCount);
            LOG_INFO << clientName
                     << " - Received echo len=" << echo.size()
                     << ", count=" << *recvCount
                     << " at " << recvTime.toFormattedString().c_str();
        });

    // 记录线程开始时间，用于计算本线程 QPS
    auto threadStart = std::chrono::steady_clock::now();

    client.connect();
    loop.loop();

    // 线程结束，计算耗时和 QPS
    auto threadEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> threadDuration = threadEnd - threadStart;
    double threadQps = kMessagesPerThread / threadDuration.count();
    LOG_INFO << clientName
             << " - Thread exiting after sending " << kMessagesPerThread
             << " messages and receiving " << *recvCount
             << ", duration=" << threadDuration.count() << "s"
             << ", QPS=" << threadQps;
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

    std::cout << "Client starting stress test\n";

    // —— 2. 启动多个客户端线程做压力测试 ——

    // 记录所有线程启动前的时间，用于全局 QPS 计算
    auto globalStart = std::chrono::steady_clock::now();

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

    // 所有线程结束，计算全局 QPS
    auto globalEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> totalDuration = globalEnd - globalStart;
    long totalMessages = static_cast<long>(kThreadCount) * kMessagesPerThread;
    double globalQps = totalMessages / totalDuration.count();

    std::cout << "All client threads finished\n";
    std::cout << "Total time: " << totalDuration.count() << " seconds, "
              << "Overall QPS: " << globalQps << "\n";

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
        std::cout << "Shutdown command sent, exiting client\n";
    }

    // —— 4. 停止异步日志 ——
    log.stop();
    return 0;
}