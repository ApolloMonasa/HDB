#include "hdb/storage/page/b_plus_tree_leaf_page.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab3 叶子页填空位。
// 叶子是唯一存 (key, RID) 的页面；next_page_id 存在页尾最后 4 字节。

void BPlusTreeLeafPage::Init(page_id_t page_id, page_id_t parent) {
    (void)page_id;
    (void)parent;
    throw NotImplementedException("Lab3: implement LeafPage::Init (docs/lab3-btree.md)");
}

page_id_t BPlusTreeLeafPage::GetNextPageId() const {
    throw NotImplementedException("Lab3: implement LeafPage::GetNextPageId");
}

void BPlusTreeLeafPage::SetNextPageId(page_id_t next) {
    (void)next;
    throw NotImplementedException("Lab3: implement LeafPage::SetNextPageId");
}

int BPlusTreeLeafPage::KeyIndex(const IndexKey &key, const NumericComparator<8> &comparator) const {
    (void)key;
    (void)comparator;
    throw NotImplementedException("Lab3: implement LeafPage::KeyIndex");
}

IndexKey BPlusTreeLeafPage::KeyAt(int index) const {
    (void)index;
    throw NotImplementedException("Lab3: implement LeafPage::KeyAt");
}

RID BPlusTreeLeafPage::RidAt(int index) const {
    (void)index;
    throw NotImplementedException("Lab3: implement LeafPage::RidAt");
}

int BPlusTreeLeafPage::Insert(const IndexKey &key, const RID &rid,
                              const NumericComparator<8> &comparator) {
    (void)key;
    (void)rid;
    (void)comparator;
    throw NotImplementedException("Lab3: implement LeafPage::Insert");
}

bool BPlusTreeLeafPage::Lookup(const IndexKey &key, RID *result,
                               const NumericComparator<8> &comparator) const {
    (void)key;
    (void)result;
    (void)comparator;
    throw NotImplementedException("Lab3: implement LeafPage::Lookup");
}

int BPlusTreeLeafPage::RemoveAndDeleteTuple(const IndexKey &key,
                                            const NumericComparator<8> &comparator) {
    (void)key;
    (void)comparator;
    throw NotImplementedException("Lab3: implement LeafPage::RemoveAndDeleteTuple");
}

void BPlusTreeLeafPage::MoveHalfTo(BPlusTreeLeafPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement LeafPage::MoveHalfTo");
}

void BPlusTreeLeafPage::MoveAllTo(BPlusTreeLeafPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement LeafPage::MoveAllTo");
}

void BPlusTreeLeafPage::MoveLastToFrontOf(BPlusTreeLeafPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement LeafPage::MoveLastToFrontOf");
}

void BPlusTreeLeafPage::MoveFirstToEndOf(BPlusTreeLeafPage *receiver) {
    (void)receiver;
    throw NotImplementedException("Lab3: implement LeafPage::MoveFirstToEndOf");
}

}  // namespace hdb
