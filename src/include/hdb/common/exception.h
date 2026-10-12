#pragma once

#include <stdexcept>
#include <string>

/**
 * @file exception.h
 * @brief HDB 异常体系（框架给定，无需修改）。
 *
 * 注意：异常消息一律使用英文（运行时输出到终端，
 * 避免Windows GBK 控制台下的中文乱码问题）。
 */

namespace hdb {

/// 所有 HDB 异常的基类
class HDBException : public std::runtime_error {
  public:
    explicit HDBException(const std::string &msg) : std::runtime_error(msg) {}
};

/**
 * 你会在整个框架里大量见到它：所有留给学生实现的函数，
 * 骨架阶段都抛出该异常。做完对应实验后它就不应再出现。
 */
class NotImplementedException : public HDBException {
  public:
    explicit NotImplementedException(const std::string &msg = "not implemented")
        : HDBException("[NotImplemented] " + msg) {}
};

/// 磁盘 I/O 失败（文件打不开、读写越界、fsync 失败等）
class IOException : public HDBException {
  public:
    explicit IOException(const std::string &msg) : HDBException("[IO] " + msg) {}
};

/// 缓冲池耗尽且无可驱逐帧
class OutOfMemoryException : public HDBException {
  public:
    explicit OutOfMemoryException(const std::string &msg = "all frames are pinned")
        : HDBException("[OutOfMemory] " + msg) {}
};

/// 参数非法（越界的页号、错误的槽位等）
class InvalidArgumentException : public HDBException {
  public:
    explicit InvalidArgumentException(const std::string &msg)
        : HDBException("[InvalidArgument] " + msg) {}
};

/// SQL 执行期错误（表不存在、类型不匹配、违反约束等）
class ExecutionException : public HDBException {
  public:
    explicit ExecutionException(const std::string &msg) : HDBException("[Execution] " + msg) {}
};

/// 并发控制相关（死锁回滚、违反 2PL 等）
class LockAbortException : public HDBException {
  public:
    explicit LockAbortException(const std::string &msg = "transaction aborted by lock manager")
        : HDBException("[LockAbort] " + msg) {}
};

}  // namespace hdb
