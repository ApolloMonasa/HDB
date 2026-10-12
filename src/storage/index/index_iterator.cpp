#include "hdb/storage/index/index_iterator.h"

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/exception.h"
#include "hdb/storage/page/b_plus_tree_leaf_page.h"

namespace hdb {

// 实现说明：Lab3 迭代器填空位。
// 构造/析构/拷贝移动的给定骨架管理叶子页的 pin 生命周期；
// Key/operator*/operator++ 是你的任务。

IndexIterator::IndexIterator(BufferPoolManager *bpm, Page *leaf_page, int index)
    : bpm_(bpm), leaf_page_(leaf_page), index_(index) {}

IndexIterator::~IndexIterator() {
    // 给定：析构时归还当前叶子页
    if (leaf_page_ != nullptr && bpm_ != nullptr) {
        bpm_->UnpinPage(leaf_page_->GetPageId(), false);
    }
}

IndexIterator::IndexIterator(const IndexIterator &other)
    : bpm_(other.bpm_), leaf_page_(other.leaf_page_), index_(other.index_) {
    // 给定：拷贝时再 pin 一次（引用计数语义）
    if (leaf_page_ != nullptr) {
        Page *p = bpm_->FetchPage(leaf_page_->GetPageId());
        leaf_page_ = p;
    }
}

IndexIterator &IndexIterator::operator=(const IndexIterator &other) {
    if (this != &other) {
        this->~IndexIterator();
        bpm_ = other.bpm_;
        leaf_page_ = other.leaf_page_;
        index_ = other.index_;
        if (leaf_page_ != nullptr) {
            leaf_page_ = bpm_->FetchPage(leaf_page_->GetPageId());
        }
    }
    return *this;
}

IndexIterator::IndexIterator(IndexIterator &&other) noexcept
    : bpm_(other.bpm_), leaf_page_(other.leaf_page_), index_(other.index_) {
    other.bpm_ = nullptr;
    other.leaf_page_ = nullptr;
    other.index_ = 0;
}

IndexIterator &IndexIterator::operator=(IndexIterator &&other) noexcept {
    if (this != &other) {
        this->~IndexIterator();
        bpm_ = other.bpm_;
        leaf_page_ = other.leaf_page_;
        index_ = other.index_;
        other.bpm_ = nullptr;
        other.leaf_page_ = nullptr;
        other.index_ = 0;
    }
    return *this;
}

IndexKey IndexIterator::Key() const {
    if (leaf_page_ == nullptr) {
        throw InvalidArgumentException("Key() on end iterator");
    }
    auto *leaf = reinterpret_cast<BPlusTreeLeafPage *>(leaf_page_->GetData());
    return leaf->KeyAt(index_);
}

RID IndexIterator::operator*() const {
    if (leaf_page_ == nullptr) {
        throw InvalidArgumentException("dereference end iterator");
    }
    auto *leaf = reinterpret_cast<BPlusTreeLeafPage *>(leaf_page_->GetData());
    return leaf->RidAt(index_);
}

IndexIterator &IndexIterator::operator++() {
    throw NotImplementedException("Lab3: implement IndexIterator::operator++ (docs/lab3-btree.md)");
}

}  // namespace hdb
