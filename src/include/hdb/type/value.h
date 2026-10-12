#pragma once

#include <cstdint>
#include <string>

#include "hdb/type/type_id.h"

namespace hdb {

/**
 * @brief 单个数据值——元组的一个"格子"（框架给定，无需修改）。
 *
 * 一个 Value 同一时刻只持有一种类型的值（tagged union 思想，
 * 用 C 风格 union + type_id 标签实现）。它负责三件事：
 *
 *   1. 存值        构造 / 拷贝 / 移动 / 析构（VARCHAR 走 std::string，安全）
 *   2. 序列化      SerializeTo / DeserializeFrom —— 元组进页面的字节级布局
 *   3. 运算        比较全套 + 算术四则（Lab4 表达式求值的基础）
 *
 * 序列化布局约定（与 Tuple::SerializeTo 配合）：
 *   定长类型：直接写 TypeSize() 字节（小端，即本机字节序）；
 *   VARCHAR ：[4 字节长度][内容字节] —— 因此变长值"内联"存放在元组数据区，
 *             元组不指向任何外部内存，整个元组可以整体 memcpy 进页面。
 */
class Value {
    // ValueFactory 是唯一推荐的造值入口，授予友元以访问私有构造
    friend class ValueFactory;

  public:
    Value() : type_(TypeId::INVALID), is_null_(true) {}
    ~Value() = default;

    Value(const Value &) = default;
    Value &operator=(const Value &) = default;
    Value(Value &&) noexcept = default;
    Value &operator=(Value &&) noexcept = default;

    TypeId GetType() const { return type_; }
    bool IsNull() const { return is_null_; }

    // ---- 取值（类型不符属于程序 bug，直接 abort 语义） ----
    bool GetAsBoolean() const { return boolean_; }
    int32_t GetAsInteger() const { return integer_; }
    int64_t GetAsBigint() const { return bigint_; }
    double GetAsFloat() const { return float_; }
    std::string GetAsVarchar() const { return varchar_; }

    // ---- 序列化 / 反序列化 ----

    /** @param dst 目标缓冲（元组数据区内本列的偏移处），需保证足够大 */
    void SerializeTo(char *dst) const;

    /**
     * @param type 期望类型
     * @param src  源缓冲
     * @param varchar_max 变长列的最大长度（仅校验用，可为 0 表示不校验）
     */
    static Value DeserializeFrom(TypeId type, const char *src, uint32_t varchar_max = 0);

    /** 本值序列化后占用的字节数（变长含 4 字节长度头） */
    uint32_t GetSerializedSize() const;

    // ---- 比较（NULL 参与比较一律返回 false，除 IsDistinctFrom 语义外不使用） ----
    int Compare(const Value &rhs) const;  ///< -1 / 0 / 1；含 NULL 返回 2（未定义）
    bool CompareEquals(const Value &rhs) const { return Compare(rhs) == 0; }
    bool CompareNotEquals(const Value &rhs) const { return Compare(rhs) != 0; }
    bool CompareLessThan(const Value &rhs) const { return Compare(rhs) < 0; }
    bool CompareLessThanEquals(const Value &rhs) const { return Compare(rhs) <= 0; }
    bool CompareGreaterThan(const Value &rhs) const { return Compare(rhs) > 0; }
    bool CompareGreaterThanEquals(const Value &rhs) const { return Compare(rhs) >= 0; }

    // ---- 算术（仅数值类型合法；类型提升：任一方 FLOAT -> FLOAT，
    //           否则任一方 BIGINT -> BIGINT，否则 INTEGER） ----
    Value Add(const Value &rhs) const;
    Value Subtract(const Value &rhs) const;
    Value Multiply(const Value &rhs) const;
    Value Divide(const Value &rhs) const;

    /// 输出为 SQL 风格字符串（VARCHAR 不带引号，BOOLEAN 输出 true/false）
    std::string ToString() const;

    /// 哈希（用于 HashJoin / GROUP BY / DISTINCT）
    size_t Hash() const;

    /// 相等（值语义；NULL 与任何值（含 NULL）都不相等，与 SQL 一致）
    bool operator==(const Value &other) const { return !is_null_ && !other.is_null_ && Compare(other) == 0; }
    bool operator!=(const Value &other) const { return !(*this == other); }

    /** 数值类型统一转成 double 比较/运算的辅助 */
    double CastToFloat() const;

  private:
    // 私有构造 + ValueFactory 静态工厂：保证 type_ 与 union 成员永远一致
    Value(TypeId t, bool v) : type_(t), is_null_(false), boolean_(v) {}
    Value(TypeId t, int32_t v) : type_(t), is_null_(false), integer_(v) {}
    Value(TypeId t, int64_t v) : type_(t), is_null_(false), bigint_(v) {}
    Value(TypeId t, double v) : type_(t), is_null_(false), float_(v) {}
    Value(TypeId t, std::string v) : type_(t), is_null_(false), varchar_(std::move(v)) {}

    TypeId type_;
    bool is_null_;
    union {
        bool boolean_;
        int32_t integer_;
        int64_t bigint_;
        double float_;
    };
    std::string varchar_;  // VARCHAR 专用（union 里放 string 需要 placement new，教学上不值）
};

}  // namespace hdb
