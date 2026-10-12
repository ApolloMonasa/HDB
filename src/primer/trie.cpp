#include "hdb/primer/trie.h"
#include "hdb/primer/trie_store.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab0 的填空位（与 Lab1-5 一样，实现写在 .cpp 里）。
// docs/lab0-trie.md 讲设计约定，test/primer/ 是判卷老师。

Trie::Trie(const Trie &other) {
    (void)other;
    throw NotImplementedException("Lab0: implement Trie copy constructor (docs/lab0-trie.md)");
}

Trie Trie::Put(const std::string &key, std::string value) const {
    (void)key;
    (void)value;
    throw NotImplementedException("Lab0: implement Trie::Put");
}

bool Trie::Get(const std::string &key, std::string *value) const {
    (void)key;
    (void)value;
    throw NotImplementedException("Lab0: implement Trie::Get");
}

Trie Trie::Remove(const std::string &key) const {
    (void)key;
    throw NotImplementedException("Lab0: implement Trie::Remove");
}

Trie TrieStore::Get() {
    throw NotImplementedException("Lab0: implement TrieStore::Get");
}

void TrieStore::Put(std::string key, std::string value) {
    (void)key;
    (void)value;
    throw NotImplementedException("Lab0: implement TrieStore::Put");
}

void TrieStore::Remove(const std::string &key) {
    (void)key;
    throw NotImplementedException("Lab0: implement TrieStore::Remove");
}

}  // namespace hdb
