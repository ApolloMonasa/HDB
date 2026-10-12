#pragma once

#include <unordered_map>
#include <vector>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：哈希连接执行器。
 *
 * Build：右表按右键哈希建桶；Probe：左行按左键探测。
 * 等值连接的标配（优化规则 R2 的产物）。
 * Grace 落盘变体（内存不足分批）为选做。
 */
class HashJoinExecutor : public AbstractExecutor {
  public:
    HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "HashJoin"; }

  private:
    std::unique_ptr<AbstractExecutor> left_;
    std::unique_ptr<AbstractExecutor> right_;
    std::unordered_map<std::string, std::vector<Tuple>> ht_;
    std::vector<Tuple> cur_matches_;
    Tuple cur_left_;
    size_t match_pos_{0};
};
}  // namespace hdb
