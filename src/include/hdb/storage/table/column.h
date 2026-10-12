#pragma once

#include <string>

#include "hdb/common/config.h"
#include "hdb/type/type_id.h"

namespace hdb {

/**
 * @brief 一列的元数据（框架给定）。
 *
 * Column + Schema 是"表长什么样"的完整描述：
 * 建表语句 CREATE TABLE t(a INT, b VARCHAR(20)) 会被翻译成
 * 两个 Column 装进一个 Schema——之后元组的序列化布局、
 * 查询里的列引用全都以它为准。
 */
class Column {
  public:
    /**
     * 定长列构造（INT/BIGINT/FLOAT/BOOLEAN）
     * @param name    列名（不区分大小写由上层处理，这里原样保存）
     * @param type    列类型
     */
    Column(std::string name, TypeId type) : name_(std::move(name)), type_(type), max_len_(0) {}

    /**
     * 变长列构造（VARCHAR）
     * @param max_len 最大字节数（SQL 里 VARCHAR(n) 的 n）
     */
    Column(std::string name, TypeId type, uint32_t max_len)
        : name_(std::move(name)), type_(type), max_len_(max_len) {}

    const std::string &GetName() const { return name_; }
    TypeId GetType() const { return type_; }
    uint32_t GetMaxLength() const { return max_len_; }
    column_id_t GetColumnId() const { return column_id_; }
    void SetColumnId(column_id_t id) { column_id_ = id; }

    /** 该列在元组数据区中的起始偏移（由 Schema 构造时统一计算填入） */
    uint32_t GetOffset() const { return offset_; }
    void SetOffset(uint32_t offset) { offset_ = offset; }

    /** 该列序列化后占用字节数（变长 = 4B 长度头 + 上限） */
    uint32_t GetFixedSerializedSize() const {
        return type_ == TypeId::VARCHAR ? 4 + max_len_ : TypeSize(type_);
    }

    bool IsInlined() const { return true; }  // HDB 所有值内联存放在元组内

  private:
    std::string name_;
    TypeId type_;
    uint32_t max_len_{0};
    column_id_t column_id_{0};
    uint32_t offset_{0};
};

}  // namespace hdb
