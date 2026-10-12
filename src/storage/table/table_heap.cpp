#include "hdb/storage/table/table_heap.h"

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/exception.h"
#include "hdb/storage/page/page.h"
#include "hdb/storage/page/table_page.h"
#include "hdb/storage/table/table_iterator.h"

namespace hdb {

// 实现说明：Lab2 Task 3 的填空位（Create 构造与 GetAllPageIds 为给定）。

TableHeap::TableHeap(BufferPoolManager *bpm, page_id_t first_page_id, Schema schema)
    : bpm_(bpm), first_page_id_(first_page_id), schema_(std::move(schema)) {}

TableHeap *TableHeap::Create(BufferPoolManager *bpm, Schema schema) {
    page_id_t first_pid;
    Page *page = bpm->NewPage(&first_pid);
    if (page == nullptr) {
        throw OutOfMemoryException("TableHeap::Create - no frame for first page");
    }
    auto *table_page = reinterpret_cast<TablePage *>(page->GetData());
    table_page->Init(first_pid, INVALID_PAGE_ID);
    bpm->UnpinPage(first_pid, true);
    auto *heap = new TableHeap(bpm, first_pid, std::move(schema));
    return heap;
}

TableHeap::~TableHeap() = default;

bool TableHeap::InsertTuple(const Tuple &tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab2: implement TableHeap::InsertTuple (docs/lab2-record.md)");
}

bool TableHeap::GetTuple(const RID &rid, Tuple *tuple) const {
    (void)rid;
    (void)tuple;
    throw NotImplementedException("Lab2: implement TableHeap::GetTuple");
}

bool TableHeap::MarkDelete(const RID &rid) {
    (void)rid;
    throw NotImplementedException("Lab2: implement TableHeap::MarkDelete");
}

bool TableHeap::UpdateTuple(const Tuple &tuple, const RID &rid, RID *new_rid) {
    (void)tuple;
    (void)rid;
    (void)new_rid;
    throw NotImplementedException("Lab2: implement TableHeap::UpdateTuple");
}

TableIterator TableHeap::Begin() const {
    throw NotImplementedException("Lab2: implement TableHeap::Begin (via TableIterator)");
}

TableIterator TableHeap::End() const {
    throw NotImplementedException("Lab2: implement TableHeap::End");
}

std::vector<page_id_t> TableHeap::GetAllPageIds() const {
    // 给定实现：测试用来检查页面链
    std::vector<page_id_t> ids;
    page_id_t pid = first_page_id_;
    while (pid != INVALID_PAGE_ID) {
        ids.push_back(pid);
        Page *page = bpm_->FetchPage(pid);
        if (page == nullptr) {
            break;
        }
        auto *tp = reinterpret_cast<TablePage *>(page->GetData());
        page_id_t next = tp->GetNextPageId();
        bpm_->UnpinPage(pid, false);
        pid = next;
    }
    return ids;
}

}  // namespace hdb
