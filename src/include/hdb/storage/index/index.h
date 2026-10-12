#pragma once

#include <string>
#include <vector>

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/index/generic_key.h"
#include "hdb/storage/table/schema.h"

namespace hdb {

/**
 * @brief 索引抽象接口（框架给定）。
 *
 * Catalog 里登记的每个索引（如 CREATE TABLE 的 PRIMARY KEY）
 * 都以 Index* 的面目出现；执行器（IndexScan）只认这个接口，
 * 不关心背后是 B+ 树还是哈希——这是"接口与实现分离"的直接收益。
 *
 * HDB 的索引值统一为 RID（记录地址）；键为单列数值（见 generic_key.h）。
 */
class Index {
  public:
    Index(std::string name, IndexKey key, Schema key_schema)
        : name_(std::move(name)), key_schema_(std::move(key_schema)) {
        (void)key;  // 键规格保留字段，暂不使用
    }

    virtual ~Index() = default;

    /** 插入 (key, rid)。已存在相同 key 时返回 false（唯一索引语义） */
    virtual bool InsertEntry(const IndexKey &key, const RID &rid) = 0;

    /** 精确查找：所有匹配 key 的 rid 追加进 result */
    virtual void ScanKey(const IndexKey &key, std::vector<RID> *result) = 0;

    /** 删除 (key, rid)。 */
    virtual bool DeleteEntry(const IndexKey &key, const RID &rid) = 0;

    const std::string &GetName() const { return name_; }
    const Schema &GetKeySchema() const { return key_schema_; }

  protected:
    std::string name_;
    Schema key_schema_;
};

}  // namespace hdb
