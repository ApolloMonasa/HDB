#pragma once

#include <string>
#include <utility>

#include "hdb/type/type_id.h"
#include "hdb/type/value.h"

namespace hdb {

/**
 * @brief 造值的唯一推荐入口（框架给定）。
 *
 * Value 的构造函数是私有的——这是刻意为之：
 * 强制所有代码通过具名工厂函数造值，类型标签与 union 成员
 * 永远不会配错（比如把 int32 塞进 BIGINT 的 union 槽位）。
 */
class ValueFactory {
  public:
    static Value CreateBoolean(bool v) { return Value(TypeId::BOOLEAN, v); }
    static Value CreateInteger(int32_t v) { return Value(TypeId::INTEGER, v); }
    static Value CreateBigint(int64_t v) { return Value(TypeId::BIGINT, v); }
    static Value CreateFloat(double v) { return Value(TypeId::FLOAT, v); }
    static Value CreateVarchar(std::string v) { return Value(TypeId::VARCHAR, std::move(v)); }
    /// NULL 值（类型标签为 INVALID，参与比较/算术时按 NULL 语义传染）
    static Value CreateNull() { return Value(); }
};

}  // namespace hdb
