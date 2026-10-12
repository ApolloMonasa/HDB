#include "hdb/storage/table/tuple.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab2 Task 1 的填空位。当前抛 NotImplementedException。

Tuple::Tuple(std::vector<Value> values, const Schema &schema) {
    (void)values;
    (void)schema;
    throw NotImplementedException("Lab2: implement Tuple(values, schema) constructor (docs/lab2-record.md)");
}

uint32_t Tuple::SerializeTo(char *dst) const {
    (void)dst;
    throw NotImplementedException("Lab2: implement Tuple::SerializeTo");
}

void Tuple::DeserializeFrom(const char *src, const Schema &schema, const RID &rid) {
    (void)src;
    (void)schema;
    (void)rid;
    throw NotImplementedException("Lab2: implement Tuple::DeserializeFrom");
}

Value Tuple::GetValue(const Schema &schema, column_id_t col_id) const {
    (void)schema;
    (void)col_id;
    throw NotImplementedException("Lab2: implement Tuple::GetValue");
}

std::string Tuple::ToString(const Schema &schema) const {
    // 给定实现：依赖你实现的 GetValue；输出 (v1, v2, ...) 形式
    std::string out = "(";
    for (uint32_t i = 0; i < schema.GetColumnCount(); i++) {
        if (i > 0) {
            out += ", ";
        }
        out += GetValue(schema, i).ToString();
    }
    return out + ")";
}

}  // namespace hdb
