#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/config.h"
#include "hdb/storage/index/b_plus_tree.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/table_heap.h"

namespace hdb {

/**
 * Catalog 里一张表的登记项。
 * heap/pk_index 是裸指针：所有权在 Catalog，计划节点按值拷贝本结构
 * 只是为了携带轻量句柄（BusTub 同款设计）。
 */
struct TableInfo {
    TableInfo() = default;
    TableInfo(const TableInfo &) = default;  // 浅拷贝：指针由 Catalog 保活
    ~TableInfo() = default;

    table_oid_t oid{0};
    std::string name;
    Schema schema;
    TableHeap *heap{nullptr};
    std::string primary_key;       // 主键列名（可空）
    BPlusTree *pk_index{nullptr};  // 主键索引（有主键时自动建）
};

/**
 * @brief 系统目录（框架给定）——数据库的"户口本"。
 *
 * Binder/Planner 拿 SQL 里的表名列名来这里换 OID、Schema、TableHeap；
 * 执行器拿到的是换好的句柄。CREATE/DROP TABLE 在这里登记注销。
 *
 * 内存版实现（表不落盘重启即失）：HDB 骨架阶段数据经 DiskManager
 * 持久化，目录本身重启后由用户重新 CREATE——恢复目录属日志恢复
 * 课题，超出本课程范围（在 architecture.md 里有说明）。
 */
class Catalog {
  public:
    explicit Catalog(BufferPoolManager *bpm) : bpm_(bpm) {}

    ~Catalog();

    /** 建表（含主键则自动建索引）。重名抛异常 */
    TableInfo *CreateTable(const std::string &name, const std::vector<Column> &columns,
                           const std::string &primary_key = "");

    /** 删表（含其页面与索引） */
    void DropTable(const std::string &name);

    /** 取表；不存在抛异常 */
    TableInfo *GetTable(const std::string &name);

    bool HasTable(const std::string &name) const {
        return name_to_table_.find(name) != name_to_table_.end();
    }

    const std::vector<std::unique_ptr<TableInfo>> &GetAllTables() const { return tables_; }

  private:
    BufferPoolManager *bpm_;
    std::vector<std::unique_ptr<TableInfo>> tables_;
    std::unordered_map<std::string, table_oid_t> name_to_table_;
    table_oid_t next_oid_{0};
};

}  // namespace hdb
