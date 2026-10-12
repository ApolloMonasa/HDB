#include "hdb/storage/table/table_iterator.h"

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/exception.h"
#include "hdb/storage/page/page.h"
#include "hdb/storage/page/table_page.h"
#include "hdb/storage/table/table_heap.h"

namespace hdb {

// 实现说明：Lab2 Task 4 的填空位。
// 拷贝/析构的给定骨架已处理好；operator* 与 operator++ 是你的任务。

TableIterator::TableIterator(const TableHeap *heap, RID rid) : heap_(heap), rid_(rid) {}

TableIterator::~TableIterator() = default;

TableIterator::TableIterator(const TableIterator &other) = default;
TableIterator &TableIterator::operator=(const TableIterator &other) = default;

Tuple TableIterator::operator*() const {
    throw NotImplementedException("Lab2: implement TableIterator::operator* (docs/lab2-record.md)");
}

TableIterator &TableIterator::operator++() {
    throw NotImplementedException("Lab2: implement TableIterator::operator++");
}

}  // namespace hdb
