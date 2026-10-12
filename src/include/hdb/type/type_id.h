#pragma once

#include <cstdint>
#include <string>

namespace hdb {

/**
 * @brief HDB 支持的全部列类型（框架给定）。
 *
 * 设计取舍：比 BusTub 18 个类型的体系精简得多，
 * 但比 BusTub 查询阶段实际使用的 INT/VARCHAR 丰富，
 * 足以覆盖课程实验需要的建表语句。
 */
enum class TypeId : uint8_t {
    INVALID = 0,  ///< 非法占位
    BOOLEAN,      ///< true / false
    INTEGER,      ///< 32 位有符号整数（SQL 的 INT）
    BIGINT,       ///< 64 位有符号整数
    FLOAT,        ///< 双精度浮点（SQL 的 FLOAT/DOUBLE）
    VARCHAR,      ///< 变长字符串（建表时带最大长度 VARCHAR(n)）
};

/// 定长类型在元组里占据的字节数；变长类型返回 0（按 [4B 长度][内容] 布局）
inline uint32_t TypeSize(const TypeId type) {
    switch (type) {
        case TypeId::BOOLEAN:
            return 1;
        case TypeId::INTEGER:
            return 4;
        case TypeId::BIGINT:
            return 8;
        case TypeId::FLOAT:
            return 8;
        default:
            return 0;  // VARCHAR / INVALID
    }
}

/// 类型名（错误消息与 EXPLAIN 输出用）
inline std::string TypeToString(const TypeId type) {
    switch (type) {
        case TypeId::BOOLEAN:
            return "BOOLEAN";
        case TypeId::INTEGER:
            return "INTEGER";
        case TypeId::BIGINT:
            return "BIGINT";
        case TypeId::FLOAT:
            return "FLOAT";
        case TypeId::VARCHAR:
            return "VARCHAR";
        default:
            return "INVALID";
    }
}

}  // namespace hdb
