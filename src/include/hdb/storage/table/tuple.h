#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hdb/common/rid.h"
#include "hdb/storage/table/schema.h"
#include "hdb/type/value.h"

namespace hdb {

/**
 * @file tuple.h
 * @brief Lab 2 Task 1：元组——"关系表的一行"的字节级真相。
 *
 * 一个 Tuple 就是一段自包含的字节流 data_ 加上它的地址 rid_：
 *
 *   ┌────────────┬────────────┬────────────┬─────┐
 *   │ col0 bytes │ col1 bytes │ col2 bytes │ ... │   布局由 Schema 决定
 *   └────────────┴────────────┴────────────┴─────┘
 *    offset 由 Schema 构造时算好；VARCHAR 处再嵌 [4B len][content]
 *
 * 因为"自包含"（不指向任何外部内存），一个 Tuple 可以被整体
 * memcpy 进 TablePage 的数据区——这正是页面存储无指针化的前提。
 *
 * 你的任务：SerializeTo / DeserializeFrom / GetValue 三个函数，
 * 它们是"逻辑行 ←→ 字节块"翻译器的全部。
 */
class Tuple {
  public:
    Tuple() = default;

    /** 用一组值按 schema 摆好布局（内存中的"新鲜"元组，尚无 RID） */
    Tuple(std::vector<Value> values, const Schema &schema);

    /** 从页面字节区反序列化出元组（搭配 TablePage::GetTuple 使用） */
    Tuple(const char *data, uint32_t size, const RID &rid) {
        data_.assign(data, data + size);
        rid_ = rid;
    }

    Tuple(const Tuple &) = default;
    Tuple &operator=(const Tuple &) = default;
    Tuple(Tuple &&) noexcept = default;
    Tuple &operator=(Tuple &&) noexcept = default;

    /** 元组序列化后的字节数 */
    uint32_t GetLength() const { return static_cast<uint32_t>(data_.size()); }

    const char *GetData() const { return data_.data(); }

    const RID &GetRid() const { return rid_; }
    void SetRid(const RID &rid) { rid_ = rid; }

    /**
     * TODO(Lab2): 把本元组写进 dst 缓冲（TablePage 的数据区空隙）。
     * 提示：逐列取 Value，按 Schema 的 offset 定位，Value::SerializeTo 落字节。
     * 变长列写完后，本函数返回实际写入长度（供调用方登记 slot）。
     */
    uint32_t SerializeTo(char *dst) const;

    /**
     * TODO(Lab2): 从 src 反序列化（TablePage 读出后重建元组）。
     * @param schema 布局依据
     * @param rid    元组的地址（登记进 rid_）
     * 提示：定长列直接按 offset 取；VARCHAR 处嵌着 [4B len]，
     *       实际占用的字节数不等于 Schema 的上限——这也是为什么
     *       Tuple 需要单独记录自己的长度。
     */
    void DeserializeFrom(const char *src, const Schema &schema, const RID &rid);

    /**
     * TODO(Lab2): 取第 col_id 列的值。
     * 提示：按 Schema 找到列 offset，Value::DeserializeFrom 处嵌套布局。
     */
    Value GetValue(const Schema &schema, column_id_t col_id) const;

    /** 调试用：按 schema 逐列打印（形如 (1, 'a', 2.5)） */
    std::string ToString(const Schema &schema) const;

    bool operator==(const Tuple &other) const { return rid_ == other.rid_ && data_ == other.data_; }

  private:
    std::vector<char> data_;
    RID rid_{};
};

}  // namespace hdb
