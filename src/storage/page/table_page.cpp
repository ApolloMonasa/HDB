#include "hdb/storage/page/table_page.h"

#include "hdb/common/exception.h"
#include "hdb/storage/page/page.h"

namespace hdb {

// 使用方式提醒：TablePage 永远不会被 new 出来，而是
//   auto *table_page = reinterpret_cast<TablePage *>(page->GetData());
// 把缓冲池某帧的数据区"当作"一个 TablePage 使用（BusTub 同款做法）。
// 因此本类不得有任何非静态数据成员——所有状态都写在那 4KB 字节里。

char *TablePage::GetDataPtr() { return reinterpret_cast<char *>(this); }
const char *TablePage::GetDataPtr() const { return reinterpret_cast<const char *>(this); }

// ---------------------------------------------------------------------------
// 以下 5 个函数是 Lab2 Task 2 的填空位
// ---------------------------------------------------------------------------

bool TablePage::InsertTuple(const Tuple &tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab2: implement TablePage::InsertTuple (docs/lab2-record.md)");
}

bool TablePage::GetTuple(const RID &rid, Tuple *tuple, const Schema &schema) {
    (void)rid;
    (void)tuple;
    (void)schema;
    throw NotImplementedException("Lab2: implement TablePage::GetTuple");
}

bool TablePage::MarkDelete(const RID &rid) {
    (void)rid;
    throw NotImplementedException("Lab2: implement TablePage::MarkDelete");
}

bool TablePage::UpdateTupleInPlace(const Tuple &tuple, const RID &rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab2: implement TablePage::UpdateTupleInPlace");
}

RID TablePage::GetNextTupleRid(const RID &rid) {
    (void)rid;
    throw NotImplementedException("Lab2: implement TablePage::GetNextTupleRid");
}

}  // namespace hdb
