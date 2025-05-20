#pragma once

#include <functional>
#include <memory>

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, const char *data, ssize_t len)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;