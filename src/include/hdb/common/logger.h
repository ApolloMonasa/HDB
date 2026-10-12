#pragma once

#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace hdb {

/**
 * @brief 极简线程安全日志（框架给定）。
 *
 * 零第三方依赖的代价就是没有 fmt/spdlog 的花活，
 * 但数据库实验里日志只需要一件事：多线程打印不串行交错。
 * 输出到 stderr（与测试结果文件分开，避免污染 sqllogictest 比对）。
 *
 * 用法：HDB_LOG_INFO("opened file %s, pages=%d", path, n);
 */
enum class LogLevel { INFO, WARN, ERROR_ };

inline void LogImpl(LogLevel level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
inline void LogImpl(LogLevel level, const char *fmt, ...) {
    static std::mutex io_mu;
    static const char *names[] = {"[info] ", "[warn] ", "[error] "};
    va_list ap;
    va_start(ap, fmt);
    std::lock_guard<std::mutex> lk(io_mu);
    std::fputs(names[static_cast<int>(level)], stderr);
    std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr);
    va_end(ap);
}

}  // namespace hdb

#define HDB_LOG_INFO(...) ::hdb::LogImpl(::hdb::LogLevel::INFO, __VA_ARGS__)
#define HDB_LOG_WARN(...) ::hdb::LogImpl(::hdb::LogLevel::WARN, __VA_ARGS__)
#define HDB_LOG_ERROR(...) ::hdb::LogImpl(::hdb::LogLevel::ERROR_, __VA_ARGS__)
