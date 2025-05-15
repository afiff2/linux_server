#pragma once

#include <unistd.h>
#include <sys/syscall.h>
namespace CurrentThread //保存tid缓存 避免反复使用系统调用
{
    extern thread_local int t_cachedTid; //如果在头文件中直接初始化 thread_local 变量，每个 .cpp 文件都会生成一个独立的定义，违反 ODR。

    void cacheTid();

    inline int tid() // 内联函数只在当前文件中起作用，必需定义在.h才能在外部访问
    {
        if (__builtin_expect(t_cachedTid == 0, 0)) // 编译器优化，条件为t_cachedTid == 0，编译器认为该条件大概率不成立
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}