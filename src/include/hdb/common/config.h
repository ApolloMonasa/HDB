#pragma once

#include <cstdint>

/**
 * @file config.h
 * @brief HDB 全局配置与类型别名 —— 整个内核的"度量衡"。
 *
 * 数据库内核里大量代码在传递"页号、帧号、槽号、事务号"这类整数 ID。
 * 如果全部用 int，一次 page_id 传给 frame_id 的错误编译器无法发现。
 * 因此我们从第一天起就为每种 ID 起一个别名（本质仍是整数，
 * 但代码可读性和 review 效率完全不同）。
 */

namespace hdb {

/** HDB 版本号（语义化版本，骨架期 0.x） */
inline constexpr const char *HDB_VERSION = "0.1.0";

/**
 * 页大小：4KB。
 * BusTub 用 8KB，HDB 选择 4KB 以对齐教材中"磁盘块 / 页"的经典描述，
 * 同时 4096 恰是多数文件系统与内存页的大小，便于建立直觉。
 */
inline constexpr int PAGE_SIZE = 4096;

/** 无效页号（0 是合法页号，因此用 -1 表示"不存在"） */
inline constexpr int32_t INVALID_PAGE_ID = -1;

/** 缓冲池默认容量（帧数），可在 BufferPoolManager 构造时覆盖 */
inline constexpr int DEFAULT_POOL_SIZE = 64;

// ---------------------------------------------------------------------------
// 全局 ID 类型别名（全部为有符号整数，与 BusTub 保持一致）
// ---------------------------------------------------------------------------
using page_id_t = int32_t;    ///< 磁盘页号
using frame_id_t = int32_t;   ///< 缓冲池帧号
using slot_id_t = int32_t;    ///< 页内槽位号（记录在页内的下标）
using txn_id_t = int64_t;     ///< 事务号
using table_oid_t = int32_t;  ///< 表对象 ID（object id）
using index_oid_t = int32_t;  ///< 索引对象 ID
using column_id_t = uint32_t; ///< 模式中的列下标

}  // namespace hdb
