#include "hdb/storage/index/b_plus_tree.h"

#include <sstream>

#include "hdb/common/exception.h"
#include "hdb/storage/page/b_plus_tree_header_page.h"
#include "hdb/storage/page/b_plus_tree_internal_page.h"
#include "hdb/storage/page/b_plus_tree_leaf_page.h"
#include "hdb/storage/page/page.h"

namespace hdb {

// 实现说明：Lab3 主战场。Init/DrawBPlusTree 为给定；
// InsertEntry / ScanKey / DeleteEntry / Begin / GetRootPageId 是你的任务。

BPlusTree::BPlusTree(BufferPoolManager *bpm, TypeId key_type, std::string name, Schema key_schema)
    : Index(std::move(name), IndexKey(), std::move(key_schema)), bpm_(bpm),
      comparator_(key_type) {}

void BPlusTree::Init() {
    throw NotImplementedException("Lab3: implement BPlusTree::Init (docs/lab3-btree.md)");
}

bool BPlusTree::InsertEntry(const IndexKey &key, const RID &rid) {
    (void)key;
    (void)rid;
    throw NotImplementedException("Lab3: implement BPlusTree::InsertEntry");
}

void BPlusTree::ScanKey(const IndexKey &key, std::vector<RID> *result) {
    (void)key;
    (void)result;
    throw NotImplementedException("Lab3: implement BPlusTree::ScanKey");
}

bool BPlusTree::DeleteEntry(const IndexKey &key, const RID &rid) {
    (void)key;
    (void)rid;
    throw NotImplementedException("Lab3: implement BPlusTree::DeleteEntry");
}

IndexIterator BPlusTree::Begin(const IndexKey &key) {
    (void)key;
    throw NotImplementedException("Lab3: implement BPlusTree::Begin(key)");
}

IndexIterator BPlusTree::Begin() {
    throw NotImplementedException("Lab3: implement BPlusTree::Begin()");
}

IndexIterator BPlusTree::End() { return IndexIterator(bpm_, nullptr, 0); }

page_id_t BPlusTree::GetRootPageId() {
    throw NotImplementedException("Lab3: implement BPlusTree::GetRootPageId");
}

bool BPlusTree::IsEmpty() { return GetRootPageId() == INVALID_PAGE_ID; }

// 给定实现：BFS 打印整棵树（文本画像）。测试与 printer 工具复用此函数。
std::string BPlusTree::DrawBPlusTree() {
    std::ostringstream out;
    out << "/" << GetName() << "\n";
    page_id_t root = GetRootPageId();
    if (root == INVALID_PAGE_ID) {
        out << "(empty tree)\n";
        return out.str();
    }
    std::queue<page_id_t> todo;
    todo.push(root);
    int level = 0;
    while (!todo.empty()) {
        size_t n = todo.size();
        out << "level " << level++ << " [";
        for (size_t i = 0; i < n; i++) {
            page_id_t pid = todo.front();
            todo.pop();
            Page *page = bpm_->FetchPage(pid);
            if (page == nullptr) {
                continue;
            }
            auto *tree_page = reinterpret_cast<BPlusTreePage *>(page->GetData());
            if (tree_page->IsLeafPage()) {
                auto *leaf = reinterpret_cast<BPlusTreeLeafPage *>(page->GetData());
                out << "leaf(" << pid << "){";
                for (int k = 0; k < leaf->GetSize(); k++) {
                    out << leaf->KeyAt(k).DeserializeTo(comparator_.GetKeyType()).ToString();
                    if (k + 1 < leaf->GetSize()) {
                        out << ",";
                    }
                }
                out << "}";
            } else {
                auto *internal = reinterpret_cast<BPlusTreeInternalPage *>(page->GetData());
                out << "internal(" << pid << "){";
                for (int k = 1; k <= internal->GetSize(); k++) {
                    out << internal->KeyAt(k).DeserializeTo(comparator_.GetKeyType()).ToString();
                    if (k < internal->GetSize()) {
                        out << ",";
                    }
                    todo.push(internal->ValueAt(k));
                }
                out << "}";
                todo.push(internal->ValueAt(0));
            }
            if (i + 1 < n) {
                out << " ";
            }
            bpm_->UnpinPage(pid, false);
        }
        out << "]\n";
    }
    return out.str();
}

}  // namespace hdb
