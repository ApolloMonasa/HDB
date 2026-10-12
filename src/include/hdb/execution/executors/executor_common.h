#pragma once

#include <string>
#include <vector>

#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/value.h"

namespace hdb {

/**
 * @file executor_common.h
 * @brief 执行器公共辅助（框架给定）。
 *
 * 两个高频小函数：
 *   * ProjectTuple：按目标 schema 的列名从源元组取值重构（投影）。
 *     约定：投影列必须在源 schema 中按名可查（planner 已保证无歧义）。
 *   * MergeTuples：连接时把左右元组拼成合并 schema 的新元组。
 */
inline Tuple ProjectTuple(const Tuple &t, const Schema &from, const Schema &to) {
    if (to.GetColumnCount() == from.GetColumnCount()) {
        bool same = true;
        for (uint32_t i = 0; i < to.GetColumnCount(); i++) {
            if (to.GetColumn(i).GetName() != from.GetColumn(i).GetName()) {
                same = false;
                break;
            }
        }
        if (same) {
            return t;  // 恒等投影：原样转发
        }
    }
    std::vector<Value> values;
    for (uint32_t i = 0; i < to.GetColumnCount(); i++) {
        const std::string &name = to.GetColumn(i).GetName();
        values.push_back(t.GetValue(from, from.GetColumn(name).GetColumnId()));
    }
    return Tuple(std::move(values), to);
}

/** 连接合并：out 必须是 from_left 与 from_right 的列拼接（planner 保证） */
inline Tuple MergeTuples(const Tuple &left, const Tuple &right, const Schema &from_left,
                         const Schema &from_right, const Schema &out) {
    std::vector<Value> values;
    for (uint32_t i = 0; i < from_left.GetColumnCount(); i++) {
        values.push_back(left.GetValue(from_left, i));
    }
    for (uint32_t i = 0; i < from_right.GetColumnCount(); i++) {
        values.push_back(right.GetValue(from_right, i));
    }
    (void)out;
    return Tuple(std::move(values), out);
}

/** 左右 schema 列拼接（连接算子重建合并模式用） */
inline Schema ConcatSchema(const Schema &left, const Schema &right) {
    std::vector<Column> cols = left.GetColumns();
    const std::vector<Column> &rc = right.GetColumns();
    cols.insert(cols.end(), rc.begin(), rc.end());
    return Schema(std::move(cols));
}

}  // namespace hdb
