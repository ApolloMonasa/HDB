#pragma once

#include <unordered_map>
#include <vector>

#include "hdb/execution/expressions/abstract_expression.h"

namespace hdb {

/**
 * @file agg_hash_table.h
 * @brief Lab 4：简易聚合哈希表（AggregationExecutor 的发动机）。
 *
 * 设计（对标 BusTub 的 SimpleAggregationHashTable）：
 *   * AggregateKey  = GROUP BY 各列的值序列（空序列 = 全表一个桶）
 *   * AggregateValue = 各聚合函数当前的累计值序列
 *   * map<AggregateKey, AggregateValue> 完成分组
 *
 * 学生实现：InsertCombine（新行并入桶）与各聚合初值/合并规则。
 */
struct AggregateKey {
    std::vector<Value> keys;

    bool operator==(const AggregateKey &other) const { return keys == other.keys; }
};

struct AggregateValue {
    std::vector<Value> aggs;
};

}  // namespace hdb

/// AggregateKey 的哈希（逐值哈希折叠）
template <>
struct std::hash<hdb::AggregateKey> {
    size_t operator()(const hdb::AggregateKey &k) const {
        size_t h = 0;
        for (const auto &v : k.keys) {
            h ^= v.Hash();
        }
        return h;
    }
};

namespace hdb {

class SimpleAggregationHashTable {
  public:
    SimpleAggregationHashTable(std::vector<AggType> agg_types) : agg_types_(std::move(agg_types)) {}

    /**
     * TODO(Lab4): 为新行定位（或创建）桶并并入聚合。
     * @param group_key  本行的 GROUP BY 值
     * @param row_aggs   本行各聚合函数的输入值
     *
     * 并入规则：
     *   COUNT(*) / COUNT(x)  计数 +1（COUNT(x) 遇 NULL 不计）
     *   SUM / AVG            累加（空桶初值 0；AVG 最后一步才除）
     *   MIN / MAX            与桶内当前值比较替换
     */
    void InsertCombine(const AggregateKey &group_key, const AggregateValue &row_aggs);

    /** 建桶时各聚合的初值（COUNT=0、SUM=0、MIN/MAX=输入行首值……由你定义清楚） */
    std::unordered_map<AggregateKey, AggregateValue> &GetTable() { return ht_; }

  private:
    std::vector<AggType> agg_types_;
    std::unordered_map<AggregateKey, AggregateValue> ht_;
};

}  // namespace hdb
