#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/type_id.h"
#include "hdb/type/value.h"
#include "hdb/type/value_factory.h"

namespace hdb {

/**
 * @file abstract_expression.h
 * @brief 执行期表达式体系（框架给定）——WHERE/JOIN ON/HAVING 谓词的运行时形态。
 *
 * Parser 吐出的是字符串 AST；Planner 把它翻译成本文件这些"可执行
 * 表达式"（列名已绑定成下标，求值只需元组+下标）。
 *
 * 求值接口只认两个元组槽位：
 *   left_tuple  —— 单表扫描或 join 的左表
 *   right_tuple —— join 的右表（可空）
 * 这正是 BusTub 的设计：一套表达式同时服务谓词过滤与连接条件。
 */
enum class ExpressionType {
    COLUMN_VALUE,
    CONSTANT,
    COMPARISON,
    ARITHMETIC,
    LOGIC_AND,
    LOGIC_OR,
    LOGIC_NOT,
    AGGREGATE,
};

class AbstractExpression {
  public:
    virtual ~AbstractExpression() = default;

    virtual Value Evaluate(const Tuple *left_tuple, const Tuple *right_tuple) const = 0;

    virtual ExpressionType GetType() const = 0;

    const std::vector<std::shared_ptr<AbstractExpression>> &GetChildren() const { return children_; }

  protected:
    std::vector<std::shared_ptr<AbstractExpression>> children_;
};

using AbstractExpressionRef = std::shared_ptr<AbstractExpression>;

/** 列引用：求值 = 从某个槽位的元组里取第 col_id 列 */
class ColumnValueExpression : public AbstractExpression {
  public:
    /**
     * @param tuple_idx 0 = 左槽（单表扫描也用左槽）；1 = 右槽（join 右表）
     * @param col_id    在对应表 schema 中的列下标
     */
    ColumnValueExpression(int tuple_idx, column_id_t col_id, Schema schema)
        : tuple_idx_(tuple_idx), col_id_(col_id), schema_(std::move(schema)) {}

    Value Evaluate(const Tuple *left_tuple, const Tuple *right_tuple) const override {
        const Tuple *t = tuple_idx_ == 0 ? left_tuple : right_tuple;
        return t->GetValue(schema_, col_id_);
    }

    ExpressionType GetType() const override { return ExpressionType::COLUMN_VALUE; }
    int GetTupleIdx() const { return tuple_idx_; }
    column_id_t GetColId() const { return col_id_; }

  private:
    int tuple_idx_;
    column_id_t col_id_;
    Schema schema_;
};

/** 常量 */
class ConstantValueExpression : public AbstractExpression {
  public:
    explicit ConstantValueExpression(Value val) : val_(std::move(val)) {}

    Value Evaluate(const Tuple * /*l*/, const Tuple * /*r*/) const override { return val_; }

    ExpressionType GetType() const override { return ExpressionType::CONSTANT; }
    const Value &GetValue() const { return val_; }

  private:
    Value val_;
};

/** 比较谓词：= <> < <= > >= */
class ComparisonExpression : public AbstractExpression {
  public:
    ComparisonExpression(std::string op, AbstractExpressionRef l, AbstractExpressionRef r)
        : op_(std::move(op)) {
        children_ = {std::move(l), std::move(r)};
    }

    Value Evaluate(const Tuple *l, const Tuple *r) const override {
        Value lv = children_[0]->Evaluate(l, r);
        Value rv = children_[1]->Evaluate(l, r);
        bool result;
        if (op_ == "=") {
            result = lv.CompareEquals(rv);
        } else if (op_ == "<>") {
            result = lv.CompareNotEquals(rv);
        } else if (op_ == "<") {
            result = lv.CompareLessThan(rv);
        } else if (op_ == "<=") {
            result = lv.CompareLessThanEquals(rv);
        } else if (op_ == ">") {
            result = lv.CompareGreaterThan(rv);
        } else {  // ">="
            result = lv.CompareGreaterThanEquals(rv);
        }
        return ValueFactory::CreateBoolean(result);
    }

    ExpressionType GetType() const override { return ExpressionType::COMPARISON; }
    const std::string &GetOp() const { return op_; }

  private:
    std::string op_;
};

/** 算术：+ - * / % */
class ArithmeticExpression : public AbstractExpression {
  public:
    ArithmeticExpression(std::string op, AbstractExpressionRef l, AbstractExpressionRef r)
        : op_(std::move(op)) {
        children_ = {std::move(l), std::move(r)};
    }

    Value Evaluate(const Tuple *l, const Tuple *r) const override {
        Value lv = children_[0]->Evaluate(l, r);
        Value rv = children_[1]->Evaluate(l, r);
        if (op_ == "+") {
            return lv.Add(rv);
        }
        if (op_ == "-") {
            return lv.Subtract(rv);
        }
        if (op_ == "*") {
            return lv.Multiply(rv);
        }
        if (op_ == "%") {
            int64_t li = static_cast<int64_t>(lv.CastToFloat());
            int64_t ri = static_cast<int64_t>(rv.CastToFloat());
            return ValueFactory::CreateInteger(static_cast<int32_t>(li % ri));
        }
        return lv.Divide(rv);
    }

    ExpressionType GetType() const override { return ExpressionType::ARITHMETIC; }

  private:
    std::string op_;
};

/** AND / OR / NOT 三件套（共享一个类的三种形态） */
class LogicExpression : public AbstractExpression {
  public:
    enum class Op { AND, OR, NOT };

    static AbstractExpressionRef And(AbstractExpressionRef l, AbstractExpressionRef r) {
        return std::make_shared<LogicExpression>(Op::AND, std::move(l), std::move(r));
    }
    static AbstractExpressionRef Or(AbstractExpressionRef l, AbstractExpressionRef r) {
        return std::make_shared<LogicExpression>(Op::OR, std::move(l), std::move(r));
    }
    static AbstractExpressionRef Not(AbstractExpressionRef child) {
        return std::make_shared<LogicExpression>(Op::NOT, std::move(child), nullptr);
    }

    LogicExpression(Op op, AbstractExpressionRef l, AbstractExpressionRef r) : op_(op) {
        children_.push_back(std::move(l));
        if (r != nullptr) {
            children_.push_back(std::move(r));
        }
    }

    Value Evaluate(const Tuple *l, const Tuple *r) const override {
        if (op_ == Op::NOT) {
            Value v = children_[0]->Evaluate(l, r);
            return ValueFactory::CreateBoolean(!v.GetAsBoolean());
        }
        bool lv = children_[0]->Evaluate(l, r).GetAsBoolean();
        if (op_ == Op::AND && !lv) {
            return ValueFactory::CreateBoolean(false);  // 短路
        }
        if (op_ == Op::OR && lv) {
            return ValueFactory::CreateBoolean(true);  // 短路
        }
        return ValueFactory::CreateBoolean(children_[1]->Evaluate(l, r).GetAsBoolean());
    }

    ExpressionType GetType() const override {
        return op_ == Op::AND ? ExpressionType::LOGIC_AND
               : op_ == Op::OR ? ExpressionType::LOGIC_OR
                               : ExpressionType::LOGIC_NOT;
    }

  private:
    Op op_;
};

/** 聚合函数：COUNT/SUM/MIN/MAX/AVG（出现在 AggregationExecutor 的计划里） */
enum class AggType { COUNT, SUM, MIN, MAX, AVG };

class AggregateExpression : public AbstractExpression {
  public:
    AggregateExpression(AggType func, AbstractExpressionRef child) : func_(func) {
        children_ = {std::move(child)};
    }

    /** 单行的"预聚合"：对聚合函数本身调用求值属于用法错误（聚合在执行器里完成） */
    Value Evaluate(const Tuple * /*l*/, const Tuple * /*r*/) const override {
        throw ExecutionException("AggregateExpression must be evaluated by AggregationExecutor");
    }

    ExpressionType GetType() const override { return ExpressionType::AGGREGATE; }
    AggType GetAggType() const { return func_; }
    const AbstractExpressionRef &GetChild() const { return children_[0]; }

  private:
    AggType func_;
};

}  // namespace hdb
