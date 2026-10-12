#include "hdb/storage/page/b_plus_tree_internal_page.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab3 内部页填空位。
// 注意首键 array[0].first 是占位（永不读取），size_ = 子页数 - 1。

void BPlusTreeInternalPage::Init(page_id_t page_id, page_id_t parent) {
    (void)page_id;
    (void)parent;
    throw NotImplementedException("Lab3: implement InternalPage::Init (docs/lab3-btree.md)");
}

page_id_t BPlusTreeInternalPage::Lookup(const IndexKey &key,
                                        const NumericComparator<8> &comparator) const {
    (void)key;
    (void)comparator;
    throw NotImplementedException("Lab3: implement InternalPage::Lookup");
}

IndexKey BPlusTreeInternalPage::KeyAt(int index) const {
    (void)index;
    throw NotImplementedException("Lab3: implement InternalPage::KeyAt");
}

void BPlusTreeInternalPage::SetKeyAt(int index, const IndexKey &key) {
    (void)index;
    (void)key;
    throw NotImplementedException("Lab3: implement InternalPage::SetKeyAt");
}

page_id_t BPlusTreeInternalPage::ValueAt(int index) const {
    (void)index;
    throw NotImplementedException("Lab3: implement InternalPage::ValueAt");
}

void BPlusTreeInternalPage::PopulateNewRoot(page_id_t old_node, const IndexKey &new_key,
                                            page_id_t new_node) {
    (void)old_node;
    (void)new_key;
    (void)new_node;
    throw NotImplementedException("Lab3: implement InternalPage::PopulateNewRoot");
}

int BPlusTreeInternalPage::InsertNodeAfter(int index, const IndexKey &key, page_id_t value) {
    (void)index;
    (void)key;
    (void)value;
    throw NotImplementedException("Lab3: implement InternalPage::InsertNodeAfter");
}

void BPlusTreeInternalPage::Remove(int index) {
    (void)index;
    throw NotImplementedException("Lab3: implement InternalPage::Remove");
}

void BPlusTreeInternalPage::MoveHalfTo(BPlusTreeInternalPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement InternalPage::MoveHalfTo");
}

void BPlusTreeInternalPage::MoveLastToFrontOf(BPlusTreeInternalPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement InternalPage::MoveLastToFrontOf");
}

void BPlusTreeInternalPage::MoveFirstToEndOf(BPlusTreeInternalPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement InternalPage::MoveFirstToEndOf");
}

}  // namespace hdb
