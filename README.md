# linux_server
这是我基于陈硕《Linux 多线程服务端编程》中的 muduo 网络库复现的一个高性能 C++ 网络库。本项目主要目标是学习现代 C++ 网络编程，并加入我自己的日志模块以提升性能。

## 特性
- 加入双缓冲机制的日志系统（AsyncLogging）

## 目录结构说明
```
├── app # 应用层
│ ├── client # 客户端程序
│ │ └── client.cc # 压力测试客户端
│ ├── server # 服务端程序
│ │ └── server.cc # 压力测试服务端
├── base # 基础组件
├── Logger # 日志模块
├── net # 网络模块
└── Thread # 线程封装
```
## 编译方式
```
mkdir build && cd build
cmake ..
make -j
```
