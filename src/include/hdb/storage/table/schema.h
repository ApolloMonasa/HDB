#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "hdb/common/config.h"
#include "hdb/common/exception.h"
#include "hdb/storage/table/column.h"

namespace hdb {

/**
 * @brief 表模式：列的有序集合（框架给定）。
 *
 * Schema 在构造时顺手做两件簿记：
 *   1. 给每列编号（column_id）并计算它在元组数据区的偏移——
 *      序列化布局由此一次性定死；
 *   2. 建立 列名 → 下标 的映射——Binder 把 SQL 里的列名翻译成下标用。
 */
class Schema {
  public:
    Schema() = default;  // 空 schema（计划节点占位等场景）

    explicit Schema(std::vector<Column> columns) : columns_(std::move(columns)) {
        uint32_t offset = GetNullBitmapSize();  // 头部先留出 NULL 位图（每列 1 bit）
        for (uint32_t i = 0; i < columns_.size(); i++) {
            columns_[i].SetColumnId(i);
            columns_[i].SetOffset(offset);
            offset += columns_[i].GetFixedSerializedSize();
        }
        for (const auto &col : columns_) {
            name_to_id_[col.GetName()] = col.GetColumnId();
        }
        serialized_size_ = offset;
    }

    /** NULL 位图字节数：ceil(列数 / 8)，第 i 位 = 1 表示第 i 列为 NULL */
    uint32_t GetNullBitmapSize() const {
        return (static_cast<uint32_t>(columns_.size()) + 7) / 8;
    }
    bool ColumnIsNull(const char *null_bitmap, column_id_t col_id) const {
        return (static_cast<uint8_t>(null_bitmap[col_id / 8]) & (1U << (col_id % 8))) != 0;
    }
    void SetColumnNull(char *null_bitmap, column_id_t col_id, bool is_null) const {
        if (is_null) {
            null_bitmap[col_id / 8] = static_cast<char>(static_cast<uint8_t>(null_bitmap[col_id / 8]) | (1U << (col_id % 8)));
        } else {
            null_bitmap[col_id / 8] = static_cast<char>(static_cast<uint8_t>(null_bitmap[col_id / 8]) & ~(1U << (col_id % 8)));
        }
    }

    const Column &GetColumn(column_id_t col_id) const { return columns_.at(col_id); }

    /** 按列名取列；不存在抛异常（Binder 上报"列不存在"错误） */
    const Column &GetColumn(const std::string &col_name) const {
        auto it = name_to_id_.find(col_name);
        if (it == name_to_id_.end()) {
            throw ExecutionException("column not found: " + col_name);
        }
        return columns_.at(it->second);
    }

    bool HasColumn(const std::string &col_name) const {
        return name_to_id_.find(col_name) != name_to_id_.end();
    }

    uint32_t GetColumnCount() const { return static_cast<uint32_t>(columns_.size()); }
    const std::vector<Column> &GetColumns() const { return columns_; }

    /** 一个符合本模式的元组序列化后的最大长度（定长部分，含变长上限） */
    uint32_t GetMaxTupleSize() const { return serialized_size_; }

  private:
    std::vector<Column> columns_;
    std::unordered_map<std::string, column_id_t> name_to_id_;
    uint32_t serialized_size_{0};
};

}  // namespace hdb
