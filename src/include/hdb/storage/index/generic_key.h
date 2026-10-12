#pragma once

#include <cstring>

#include "hdb/type/type_id.h"
#include "hdb/type/value.h"

namespace hdb {

/**
 * @brief 索引键的通用定长表示（框架给定）。
 *
 * 为什么索引不直接用 Value？因为 Value 是变长的（VARCHAR 带 std::string），
 * 而 B+ 树页面里要摆**定长**的键数组（offset 可算、二分可查）。
 * GenericKey<N> 就是"把任意键序列化进 N 字节定长缓冲"的容器。
 *
 * HDB 约定：索引键只支持数值类型（INTEGER/BIGINT/FLOAT），
 * 即 N = 8 足够（见 NumericComparator 的比较方式说明）。
 */
template <size_t N>
class GenericKey {
  public:
    void SerializeFrom(const Value &value) { value.SerializeTo(data_); }

    Value DeserializeTo(TypeId type) const { return Value::DeserializeFrom(type, data_); }

    const char *GetData() const { return data_; }
    char *GetData() { return data_; }

    bool operator==(const GenericKey &other) const {
        return std::memcmp(data_, other.data_, N) == 0;
    }

  private:
    char data_[N]{};
};

/** HDB 统一使用的索引键规格：8 字节定长 */
using IndexKey = GenericKey<8>;

/**
 * @brief 数值键比较器（框架给定）。
 *
 * B+ 树是模板类，键的"大小关系"通过比较器注入（BusTub 同款设计）。
 * 比较逻辑：反序列化成 Value 后用 Value::Compare——
 * 直接 memcmp 字节对负数/浮点是错的，这是常见的入门坑。
 */
template <size_t N>
class NumericComparator {
  public:
    explicit NumericComparator(TypeId key_type) : key_type_(key_type) {}

    int operator()(const GenericKey<N> &lhs, const GenericKey<N> &rhs) const {
        return lhs.DeserializeTo(key_type_).Compare(rhs.DeserializeTo(key_type_));
    }

    TypeId GetKeyType() const { return key_type_; }

  private:
    TypeId key_type_;
};

}  // namespace hdb
