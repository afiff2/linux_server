#pragma once

#include <functional>
#include <memory>
#include "Buffer.h"

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer* buf, ssize_t len)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;