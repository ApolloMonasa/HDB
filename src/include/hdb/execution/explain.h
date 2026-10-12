#pragma once

#include <string>

#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @file explain.h
 * @brief 计划树渲染（框架给定）——EXPLAIN 命令的输出器。
 *
 * 递归打印算子树：每层缩进两格，标注算子类型与关键参数。
 * 是学生调试 Lab4 优化规则（R1/R2 是否触发）的主要观察窗口。
 */
inline std::string PlanToString(const AbstractPlanNode *plan, int depth = 0) {
    if (plan == nullptr) {
        return "";
    }
    std::string indent(static_cast<size_t>(depth) * 2, ' ');
    std::string out;
    switch (plan->GetType()) {
        case PlanType::SEQ_SCAN: {
            const auto *p = static_cast<const SeqScanPlanNode *>(plan);
            out += indent + "SeqScan(table=" + p->GetTable().name;
            if (p->GetFilter() != nullptr) {
                out += ", filter=on";
            }
            out += ")\n";
            break;
        }
        case PlanType::INDEX_SCAN: {
            const auto *p = static_cast<const IndexScanPlanNode *>(plan);
            out += indent + "IndexScan(table=" + p->GetTable().name + ", index=" +
                   p->GetTable().primary_key + ")\n";
            break;
        }
        case PlanType::INSERT: {
            const auto *p = static_cast<const InsertPlanNode *>(plan);
            out += indent + "Insert(table=" + p->GetTable().name + ", rows=" +
                   std::to_string(p->GetRows().size()) + ")\n";
            break;
        }
        case PlanType::DELETE_:
            out += indent + "Delete\n";
            break;
        case PlanType::UPDATE:
            out += indent + "Update\n";
            break;
        case PlanType::NESTED_LOOP_JOIN:
            out += indent + "NestedLoopJoin\n";
            break;
        case PlanType::HASH_JOIN:
            out += indent + "HashJoin\n";
            break;
        case PlanType::AGGREGATION:
            out += indent + "Aggregation\n";
            break;
        case PlanType::SORT:
            out += indent + "Sort\n";
            break;
        case PlanType::LIMIT: {
            const auto *p = static_cast<const LimitPlanNode *>(plan);
            out += indent + "Limit(n=" + std::to_string(p->GetLimit()) + ")\n";
            break;
        }
        case PlanType::TOPN: {
            const auto *p = static_cast<const TopNPlanNode *>(plan);
            out += indent + "TopN(n=" + std::to_string(p->GetN()) + ")\n";
            break;
        }
    }
    for (size_t i = 0; i < plan->GetChildrenSize(); i++) {
        out += PlanToString(plan->GetChild(i), depth + 1);
    }
    return out;
}

}  // namespace hdb
