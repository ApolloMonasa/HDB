#include "hdb/type/value.h"

#include <cmath>
#include <cstring>
#include <functional>

#include "hdb/common/exception.h"

namespace hdb {

// ---------------------------------------------------------------------------
// 序列化：定长类型原样写，VARCHAR 写 [4B 长度][内容]
// ---------------------------------------------------------------------------
void Value::SerializeTo(char *dst) const {
    switch (type_) {
        case TypeId::BOOLEAN:
            dst[0] = boolean_ ? 1 : 0;
            break;
        case TypeId::INTEGER:
            std::memcpy(dst, &integer_, 4);
            break;
        case TypeId::BIGINT:
            std::memcpy(dst, &bigint_, 8);
            break;
        case TypeId::FLOAT:
            std::memcpy(dst, &float_, 8);
            break;
        case TypeId::VARCHAR: {
            uint32_t len = static_cast<uint32_t>(varchar_.size());
            std::memcpy(dst, &len, 4);
            std::memcpy(dst + 4, varchar_.data(), len);
            break;
        }
        default:
            throw InvalidArgumentException("serialize an INVALID value");
    }
}

Value Value::DeserializeFrom(TypeId type, const char *src, uint32_t varchar_max) {
    switch (type) {
        case TypeId::BOOLEAN:
            return Value(type, src[0] != 0);
        case TypeId::INTEGER: {
            int32_t v;
            std::memcpy(&v, src, 4);
            return Value(type, v);
        }
        case TypeId::BIGINT: {
            int64_t v;
            std::memcpy(&v, src, 8);
            return Value(type, v);
        }
        case TypeId::FLOAT: {
            double v;
            std::memcpy(&v, src, 8);
            return Value(type, v);
        }
        case TypeId::VARCHAR: {
            uint32_t len;
            std::memcpy(&len, src, 4);
            if (varchar_max != 0 && len > varchar_max) {
                throw InvalidArgumentException("varchar length exceeds max on deserialize");
            }
            return Value(type, std::string(src + 4, len));
        }
        default:
            throw InvalidArgumentException("deserialize an INVALID value");
    }
}

uint32_t Value::GetSerializedSize() const {
    switch (type_) {
        case TypeId::VARCHAR:
            return 4 + static_cast<uint32_t>(varchar_.size());
        default:
            return TypeSize(type_);
    }
}

// ---------------------------------------------------------------------------
// 比较：统一提升到"最宽容"的数值域再比，比较语义与算术提升保持一致
// ---------------------------------------------------------------------------
int Value::Compare(const Value &rhs) const {
    if (is_null_ || rhs.is_null_) {
        return 2;  // SQL 中 NULL 比较"未定义"，调用方需先检查
    }
    if (type_ == TypeId::VARCHAR || rhs.type_ == TypeId::VARCHAR) {
        if (type_ != TypeId::VARCHAR || rhs.type_ != TypeId::VARCHAR) {
            throw InvalidArgumentException("cannot compare varchar with numeric");
        }
        return varchar_.compare(rhs.varchar_);
    }
    // 数值统一转 double 比较（教学实现，接受 INT 与 FLOAT 直接可比的宽松语义）
    double l = CastToFloat(), r = rhs.CastToFloat();
    if (l < r) {
        return -1;
    }
    return l > r ? 1 : 0;
}

double Value::CastToFloat() const {
    switch (type_) {
        case TypeId::BOOLEAN:
            return boolean_ ? 1.0 : 0.0;
        case TypeId::INTEGER:
            return integer_;
        case TypeId::BIGINT:
            return static_cast<double>(bigint_);
        case TypeId::FLOAT:
            return float_;
        default:
            throw InvalidArgumentException("cast varchar to numeric");
    }
}

// ---------------------------------------------------------------------------
// 算术：NULL 传染；类型提升任一 FLOAT -> FLOAT，否则 BIGINT，否则 INTEGER
// ---------------------------------------------------------------------------
Value Value::Add(const Value &rhs) const {
    if (is_null_ || rhs.is_null_) {
        return Value();
    }
    TypeId out = (type_ == TypeId::FLOAT || rhs.type_ == TypeId::FLOAT)   ? TypeId::FLOAT
                 : (type_ == TypeId::BIGINT || rhs.type_ == TypeId::BIGINT) ? TypeId::BIGINT
                                                                            : TypeId::INTEGER;
    if (out == TypeId::INTEGER) {
        return Value(out, integer_ + rhs.integer_);
    }
    if (out == TypeId::BIGINT) {
        return Value(out, static_cast<int64_t>(CastToFloat()) + static_cast<int64_t>(rhs.CastToFloat()));
    }
    return Value(out, CastToFloat() + rhs.CastToFloat());
}

Value Value::Subtract(const Value &rhs) const {
    if (is_null_ || rhs.is_null_) {
        return Value();
    }
    TypeId out = (type_ == TypeId::FLOAT || rhs.type_ == TypeId::FLOAT)   ? TypeId::FLOAT
                 : (type_ == TypeId::BIGINT || rhs.type_ == TypeId::BIGINT) ? TypeId::BIGINT
                                                                            : TypeId::INTEGER;
    if (out == TypeId::INTEGER) {
        return Value(out, integer_ - rhs.integer_);
    }
    if (out == TypeId::BIGINT) {
        return Value(out, static_cast<int64_t>(CastToFloat()) - static_cast<int64_t>(rhs.CastToFloat()));
    }
    return Value(out, CastToFloat() - rhs.CastToFloat());
}

Value Value::Multiply(const Value &rhs) const {
    if (is_null_ || rhs.is_null_) {
        return Value();
    }
    TypeId out = (type_ == TypeId::FLOAT || rhs.type_ == TypeId::FLOAT)   ? TypeId::FLOAT
                 : (type_ == TypeId::BIGINT || rhs.type_ == TypeId::BIGINT) ? TypeId::BIGINT
                                                                            : TypeId::INTEGER;
    if (out == TypeId::INTEGER) {
        return Value(out, integer_ * rhs.integer_);
    }
    if (out == TypeId::BIGINT) {
        return Value(out, static_cast<int64_t>(CastToFloat()) * static_cast<int64_t>(rhs.CastToFloat()));
    }
    return Value(out, CastToFloat() * rhs.CastToFloat());
}

Value Value::Divide(const Value &rhs) const {
    if (is_null_ || rhs.is_null_) {
        return Value();
    }
    double r = rhs.CastToFloat();
    if (r == 0.0) {
        throw ExecutionException("division by zero");
    }
    TypeId out = (type_ == TypeId::FLOAT || rhs.type_ == TypeId::FLOAT)   ? TypeId::FLOAT
                 : (type_ == TypeId::BIGINT || rhs.type_ == TypeId::BIGINT) ? TypeId::BIGINT
                                                                            : TypeId::INTEGER;
    if (out == TypeId::INTEGER) {
        return Value(out, integer_ / rhs.integer_);
    }
    if (out == TypeId::BIGINT) {
        return Value(out, static_cast<int64_t>(CastToFloat()) / static_cast<int64_t>(r));
    }
    return Value(out, CastToFloat() / r);
}

// ---------------------------------------------------------------------------
// 展示与哈希
// ---------------------------------------------------------------------------
std::string Value::ToString() const {
    if (is_null_) {
        return "NULL";
    }
    char buf[32];
    switch (type_) {
        case TypeId::BOOLEAN:
            return boolean_ ? "true" : "false";
        case TypeId::INTEGER:
            std::snprintf(buf, sizeof(buf), "%d", integer_);
            return buf;
        case TypeId::BIGINT:
            std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(bigint_));
            return buf;
        case TypeId::FLOAT: {
            // 去掉多余尾零：1.000000 -> 1，1.500000 -> 1.5
            std::snprintf(buf, sizeof(buf), "%g", float_);
            return buf;
        }
        case TypeId::VARCHAR:
            return varchar_;
        default:
            return "INVALID";
    }
}

size_t Value::Hash() const {
    size_t h = std::hash<int>()(static_cast<int>(type_));
    if (is_null_) {
        return h;
    }
    switch (type_) {
        case TypeId::BOOLEAN:
            h ^= std::hash<bool>()(boolean_);
            break;
        case TypeId::INTEGER:
            h ^= std::hash<int32_t>()(integer_);
            break;
        case TypeId::BIGINT:
            h ^= std::hash<int64_t>()(bigint_);
            break;
        case TypeId::FLOAT:
            h ^= std::hash<double>()(float_);
            break;
        case TypeId::VARCHAR:
            h ^= std::hash<std::string>()(varchar_);
            break;
        default:
            break;
    }
    return h;
}

}  // namespace hdb
