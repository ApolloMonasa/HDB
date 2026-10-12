/**
 * tools/b_plus_tree_printer —— B+ 树可视化调试工具（框架给定）
 *
 * 用法：
 *   ./build/bin/hdb-bpt-printer [N]
 *
 * 构建一棵含 N 个键（默认 50）的示例树，输出：
 *   1. 文本画像（树形层级结构）
 *   2. graphviz DOT 语句：`hdb-bpt-printer 50 > bpt.dot && dot -Tsvg bpt.dot -o bpt.svg`
 *
 * 完成你的 Lab3 后跑它，肉眼检查分裂/兄弟链接是否正确。
 */
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <string>
#include <vector>

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/storage/index/b_plus_tree.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/page/b_plus_tree_internal_page.h"
#include "hdb/storage/page/b_plus_tree_leaf_page.h"
#include "hdb/storage/page/page.h"
#include "hdb/storage/table/column.h"
#include "hdb/type/value_factory.h"

using namespace hdb;

int main(int argc, char **argv) {
    int n = argc > 1 ? std::atoi(argv[1]) : 50;

    const std::string db_file = "/tmp/hdb_bpt_printer.db";
    std::remove(db_file.c_str());

    DiskManager dm(db_file);
    BufferPoolManager bpm(32, &dm);
    Schema key_schema({Column("k", TypeId::INTEGER)});
    auto *tree = new BPlusTree(&bpm, TypeId::BIGINT, "pk", key_schema);
    tree->Init();

    for (int i = 0; i < n; i++) {
        int64_t k = (i * 7) % (n * 3);  // 伪随机但可复现
        IndexKey key;
        key.SerializeFrom(ValueFactory::CreateInteger(static_cast<int32_t>(k)));
        tree->InsertEntry(key, RID(1, static_cast<slot_id_t>(k)));
    }

    std::puts("=== HDB B+Tree printer ===");
    std::puts("--- text profile ---");
    std::fputs(tree->DrawBPlusTree().c_str(), stdout);

    // ---- graphviz DOT 输出（BFS，与文本画像同一套遍历） ----
    std::puts("--- graphviz DOT ---");
    std::puts("digraph bpt {");
    std::puts("  node [shape=record, height=0.1];");
    page_id_t root = tree->GetRootPageId();
    if (root != INVALID_PAGE_ID) {
        std::queue<page_id_t> todo;
        todo.push(root);
        while (!todo.empty()) {
            page_id_t pid = todo.front();
            todo.pop();
            Page *page = bpm.FetchPage(pid);
            if (page == nullptr) {
                continue;
            }
            auto *tree_page = reinterpret_cast<BPlusTreePage *>(page->GetData());
            std::printf("  p%d [label=\"", pid);
            if (tree_page->IsLeafPage()) {
                auto *leaf = reinterpret_cast<BPlusTreeLeafPage *>(page->GetData());
                std::printf("<f0> leaf");
                for (int k = 0; k < leaf->GetSize(); k++) {
                    std::printf("|%s",
                                leaf->KeyAt(k).DeserializeTo(TypeId::BIGINT).ToString().c_str());
                }
                std::printf("\"];");
            } else {
                auto *internal = reinterpret_cast<BPlusTreeInternalPage *>(page->GetData());
                std::printf("<f0> *");
                for (int k = 1; k <= internal->GetSize(); k++) {
                    std::printf("|%s|<f%d> *",
                                internal->KeyAt(k).DeserializeTo(TypeId::BIGINT).ToString().c_str(), k);
                    todo.push(internal->ValueAt(k));
                }
                todo.push(internal->ValueAt(0));
                std::printf("\"];");
            }
            std::puts("");
            bpm.UnpinPage(pid, false);
        }
        // 父子边
        std::queue<page_id_t> edges;
        edges.push(root);
        while (!edges.empty()) {
            page_id_t pid = edges.front();
            edges.pop();
            Page *page = bpm.FetchPage(pid);
            if (page == nullptr) {
                continue;
            }
            auto *tree_page = reinterpret_cast<BPlusTreePage *>(page->GetData());
            if (!tree_page->IsLeafPage()) {
                auto *internal = reinterpret_cast<BPlusTreeInternalPage *>(page->GetData());
                for (int k = 0; k <= internal->GetSize(); k++) {
                    std::printf("  p%d:f%d -> p%d;\n", pid, k, internal->ValueAt(k));
                    edges.push(internal->ValueAt(k));
                }
            }
            bpm.UnpinPage(pid, false);
        }
    }
    std::puts("}");

    delete tree;
    std::remove(db_file.c_str());
    return 0;
}
