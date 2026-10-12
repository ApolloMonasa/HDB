#include "hdb/execution/executors/agg_hash_table.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab4 AggregationExecutor 的发动机填空位。
// 初值语义建议：COUNT 初值 0；SUM 初值 0（空输入 SUM 输出 0）；
// MIN/MAX 建桶时用首行值初始化（空输入无桶自然无输出）。

void SimpleAggregationHashTable::InsertCombine(const AggregateKey &group_key,
                                               const AggregateValue &row_aggs) {
    (void)group_key;
    (void)row_aggs;
    throw NotImplementedException("Lab4: implement SimpleAggregationHashTable::InsertCombine");
}

}  // namespace hdb
