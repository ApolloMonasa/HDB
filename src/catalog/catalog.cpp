#include "hdb/catalog/catalog.h"

#include "hdb/common/exception.h"
#include "hdb/storage/table/column.h"

namespace hdb {

Catalog::~Catalog() {
    for (auto &t : tables_) {
        delete t->heap;
        delete t->pk_index;
    }
}

TableInfo *Catalog::CreateTable(const std::string &name, const std::vector<Column> &columns,
                                const std::string &primary_key) {
    if (HasTable(name)) {
        throw ExecutionException("table already exists: " + name);
    }
    Schema schema(columns);

    auto info = std::make_unique<TableInfo>();
    info->oid = next_oid_++;
    info->name = name;
    info->schema = schema;
    info->heap = TableHeap::Create(bpm_, schema);
    info->primary_key = primary_key;

    if (!primary_key.empty()) {
        if (!schema.HasColumn(primary_key)) {
            throw ExecutionException("primary key column not found: " + primary_key);
        }
        TypeId key_type = schema.GetColumn(primary_key).GetType();
        auto *tree =
            new BPlusTree(bpm_, key_type, name + "__pk", Schema({schema.GetColumn(primary_key)}));
        tree->Init();
        info->pk_index = tree;
    }

    TableInfo *raw = info.get();
    name_to_table_[name] = info->oid;
    tables_.push_back(std::move(info));
    return raw;
}

void Catalog::DropTable(const std::string &name) {
    auto it = name_to_table_.find(name);
    if (it == name_to_table_.end()) {
        throw ExecutionException("table not found: " + name);
    }
    for (auto tt = tables_.begin(); tt != tables_.end(); ++tt) {
        if ((*tt)->name == name) {
            delete (*tt)->heap;
            delete (*tt)->pk_index;
            tables_.erase(tt);
            break;
        }
    }
    name_to_table_.erase(it);
}

TableInfo *Catalog::GetTable(const std::string &name) {
    auto it = name_to_table_.find(name);
    if (it == name_to_table_.end()) {
        throw ExecutionException("table not found: " + name);
    }
    for (auto &t : tables_) {
        if (t->name == name) {
            return t.get();
        }
    }
    return nullptr;  // 不可达：映射与数组同步维护
}

}  // namespace hdb
