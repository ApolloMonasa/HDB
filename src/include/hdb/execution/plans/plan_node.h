#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "hdb/catalog/catalog.h"
#include "hdb/execution/expressions/abstract_expression.h"
#include "hdb/storage/table/schema.h"

namespace hdb {

/**
 * @file plan_node.h
 * @brief 查询计划节点体系（框架给定）——优化器和执行器之间的"电路图"。
 *
 * 计划树 = 一组 PlanNode 组成的 DAG：
 *   Planner 产出 → Optimizer 改写（SeqScan→IndexScan 等）→
 *   ExecutorFactory 逐节点实例化执行器。
 */
enum class PlanType {
    SEQ_SCAN, INSERT, DELETE_, UPDATE, INDEX_SCAN,
    NESTED_LOOP_JOIN, HASH_JOIN,
    AGGREGATION, SORT, LIMIT, TOPN,
};

class AbstractPlanNode {
  public:
    AbstractPlanNode(std::vector<std::unique_ptr<AbstractPlanNode>> children, Schema output)
        : children_(std::move(children)), output_schema_(std::move(output)) {}

    virtual ~AbstractPlanNode() = default;
    virtual PlanType GetType() const = 0;

    const AbstractPlanNode *GetChild(size_t idx = 0) const { return children_.at(idx).get(); }
    size_t GetChildrenSize() const { return children_.size(); }
    const Schema &OutputSchema() const { return output_schema_; }

    /** Planner 组装投影列时重设输出模式（投影融合在扫描/连接节点上） */
    void SetOutputSchema(Schema schema) { output_schema_ = std::move(schema); }

    /**
     * 优化规则改写时**搬移**全部孩子（本节点随后应被销毁）。
     * 搬空孩子的节点再析构不会重复释放。
     */
    std::vector<std::unique_ptr<AbstractPlanNode>> TakeChildren() {
        return std::move(children_);
    }

  protected:
    /** 单孩子构造辅助（initializer_list 会拷贝 unique_ptr，不可用） */
    static std::vector<std::unique_ptr<AbstractPlanNode>> One(std::unique_ptr<AbstractPlanNode> c) {
        std::vector<std::unique_ptr<AbstractPlanNode>> v;
        v.push_back(std::move(c));
        return v;
    }
    /** 双孩子构造辅助 */
    static std::vector<std::unique_ptr<AbstractPlanNode>> Two(std::unique_ptr<AbstractPlanNode> a,
                                                              std::unique_ptr<AbstractPlanNode> b) {
        std::vector<std::unique_ptr<AbstractPlanNode>> v;
        v.push_back(std::move(a));
        v.push_back(std::move(b));
        return v;
    }

    std::vector<std::unique_ptr<AbstractPlanNode>> children_;
    Schema output_schema_;
};

// ---------------------------------------------------------------------------
// 扫描与修改
// ---------------------------------------------------------------------------
class SeqScanPlanNode : public AbstractPlanNode {
  public:
    SeqScanPlanNode(TableInfo table, AbstractExpressionRef filter, Schema output)
        : AbstractPlanNode({}, std::move(output)), table_(std::move(table)),
          filter_(std::move(filter)) {}

    PlanType GetType() const override { return PlanType::SEQ_SCAN; }
    const TableInfo &GetTable() const { return table_; }
    const AbstractExpressionRef &GetFilter() const { return filter_; }
    /** Planner 把 WHERE 谓词融合进扫描（BusTub 风格） */
    void SetFilter(AbstractExpressionRef filter) { filter_ = std::move(filter); }

  private:
    TableInfo table_;  // 值拷贝一份轻量登记项（heap 指针由 catalog 持有）
    AbstractExpressionRef filter_;
};

class InsertPlanNode : public AbstractPlanNode {
  public:
    /** 插入字面量行（INSERT ... VALUES） */
    InsertPlanNode(TableInfo table, std::vector<std::vector<Value>> rows)
        : AbstractPlanNode({}, Schema()), table_(std::move(table)), rows_(std::move(rows)) {}

    PlanType GetType() const override { return PlanType::INSERT; }
    const TableInfo &GetTable() const { return table_; }
    const std::vector<std::vector<Value>> &GetRows() const { return rows_; }

  private:
    TableInfo table_;
    std::vector<std::vector<Value>> rows_;
};

class DeletePlanNode : public AbstractPlanNode {
  public:
    DeletePlanNode(TableInfo table, std::unique_ptr<AbstractPlanNode> child)
        : AbstractPlanNode(One(std::move(child)), Schema()), table_(std::move(table)) {}

    PlanType GetType() const override { return PlanType::DELETE_; }
    const TableInfo &GetTable() const { return table_; }

  private:
    TableInfo table_;
};

class UpdatePlanNode : public AbstractPlanNode {
  public:
    UpdatePlanNode(TableInfo table, std::vector<std::pair<column_id_t, AbstractExpressionRef>> sets,
                   std::unique_ptr<AbstractPlanNode> child)
        : AbstractPlanNode(One(std::move(child)), Schema()), table_(std::move(table)),
          sets_(std::move(sets)) {}

    PlanType GetType() const override { return PlanType::UPDATE; }
    const TableInfo &GetTable() const { return table_; }
    const std::vector<std::pair<column_id_t, AbstractExpressionRef>> &GetSets() const {
        return sets_;
    }

  private:
    TableInfo table_;
    std::vector<std::pair<column_id_t, AbstractExpressionRef>> sets_;
};

class IndexScanPlanNode : public AbstractPlanNode {
  public:
    IndexScanPlanNode(TableInfo table, AbstractExpressionRef filter, Schema output)
        : AbstractPlanNode({}, std::move(output)), table_(std::move(table)),
          filter_(std::move(filter)) {}

    PlanType GetType() const override { return PlanType::INDEX_SCAN; }
    const TableInfo &GetTable() const { return table_; }
    const AbstractExpressionRef &GetFilter() const { return filter_; }

  private:
    TableInfo table_;
    AbstractExpressionRef filter_;
};

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------
enum class JoinType { INNER, LEFT };

class NestedLoopJoinPlanNode : public AbstractPlanNode {
  public:
    NestedLoopJoinPlanNode(std::unique_ptr<AbstractPlanNode> left,
                           std::unique_ptr<AbstractPlanNode> right, JoinType type,
                           AbstractExpressionRef predicate, Schema output)
        : AbstractPlanNode(Two(std::move(left), std::move(right)), std::move(output)), join_type_(type),
          predicate_(std::move(predicate)) {}

    PlanType GetType() const override { return PlanType::NESTED_LOOP_JOIN; }
    JoinType GetJoinType() const { return join_type_; }
    const AbstractExpressionRef &GetPredicate() const { return predicate_; }
    /** Planner 把 WHERE 追加为合取项（语义近似：LEFT JOIN 下 WHERE 严格讲应过滤补 NULL 行） */
    void AndPredicate(AbstractExpressionRef extra) {
        predicate_ = predicate_ == nullptr ? std::move(extra)
                                           : LogicExpression::And(predicate_, std::move(extra));
    }

  private:
    JoinType join_type_;
    AbstractExpressionRef predicate_;
};

class HashJoinPlanNode : public AbstractPlanNode {
  public:
    HashJoinPlanNode(std::unique_ptr<AbstractPlanNode> left, std::unique_ptr<AbstractPlanNode> right,
                     JoinType type, AbstractExpressionRef left_key, AbstractExpressionRef right_key,
                     Schema output)
        : AbstractPlanNode(Two(std::move(left), std::move(right)), std::move(output)), join_type_(type),
          left_key_(std::move(left_key)), right_key_(std::move(right_key)) {}

    PlanType GetType() const override { return PlanType::HASH_JOIN; }
    JoinType GetJoinType() const { return join_type_; }
    const AbstractExpressionRef &LeftKey() const { return left_key_; }
    const AbstractExpressionRef &RightKey() const { return right_key_; }

  private:
    JoinType join_type_;
    AbstractExpressionRef left_key_;
    AbstractExpressionRef right_key_;
};

// ---------------------------------------------------------------------------
// 聚合与排序
// ---------------------------------------------------------------------------
class AggregationPlanNode : public AbstractPlanNode {
  public:
    /**
     * @param having_agg_idx HAVING 谓词引用的聚合在 aggregates 里的下标
     *        （-1 表示 HAVING 不引用聚合）。HAVING 专属聚合会被追加进
     *        aggregates 参与计算但不进输出（输出按 OutputSchema 截断）。
     */
    AggregationPlanNode(std::unique_ptr<AbstractPlanNode> child,
                        std::vector<AbstractExpressionRef> group_bys,
                        std::vector<AbstractExpressionRef> aggregates,
                        AbstractExpressionRef having, Schema output, int having_agg_idx = -1)
        : AbstractPlanNode(One(std::move(child)), std::move(output)), group_bys_(std::move(group_bys)),
          aggregates_(std::move(aggregates)), having_(std::move(having)),
          having_agg_idx_(having_agg_idx) {}

    PlanType GetType() const override { return PlanType::AGGREGATION; }
    const std::vector<AbstractExpressionRef> &GetGroupBys() const { return group_bys_; }
    const std::vector<AbstractExpressionRef> &GetAggregates() const { return aggregates_; }
    const AbstractExpressionRef &GetHaving() const { return having_; }
    int GetHavingAggIdx() const { return having_agg_idx_; }

  private:
    std::vector<AbstractExpressionRef> group_bys_;
    std::vector<AbstractExpressionRef> aggregates_;
    AbstractExpressionRef having_;
    int having_agg_idx_{-1};
};

struct OrderByItem {
    AbstractExpressionRef expr;
    bool ascending = true;
    std::string alias;  // 输出列名（画表头用）
};

class SortPlanNode : public AbstractPlanNode {
  public:
    SortPlanNode(std::unique_ptr<AbstractPlanNode> child, std::vector<OrderByItem> order_bys)
        : AbstractPlanNode(One(std::move(child)), Schema()), order_bys_(std::move(order_bys)) {}

    PlanType GetType() const override { return PlanType::SORT; }
    const std::vector<OrderByItem> &GetOrderBys() const { return order_bys_; }

  private:
    std::vector<OrderByItem> order_bys_;
};

class LimitPlanNode : public AbstractPlanNode {
  public:
    LimitPlanNode(std::unique_ptr<AbstractPlanNode> child, int64_t limit)
        : AbstractPlanNode(One(std::move(child)), Schema()), limit_(limit) {}

    PlanType GetType() const override { return PlanType::LIMIT; }
    int64_t GetLimit() const { return limit_; }

  private:
    int64_t limit_;
};

class TopNPlanNode : public AbstractPlanNode {
  public:
    TopNPlanNode(std::unique_ptr<AbstractPlanNode> child, std::vector<OrderByItem> order_bys,
                 int64_t n)
        : AbstractPlanNode(One(std::move(child)), Schema()), order_bys_(std::move(order_bys)), n_(n) {}

    PlanType GetType() const override { return PlanType::TOPN; }
    const std::vector<OrderByItem> &GetOrderBys() const { return order_bys_; }
    int64_t GetN() const { return n_; }

  private:
    std::vector<OrderByItem> order_bys_;
    int64_t n_;
};

}  // namespace hdb
