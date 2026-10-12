#include "hdb/buffer/lru_k_replacer.h"

#include "hdb/common/exception.h"

namespace hdb {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : num_frames_(num_frames), k_(k) {}

// 实现说明：以下函数体是 Lab1 的填空位，当前抛出 NotImplementedException。
// 请先阅读 src/include/hdb/buffer/lru_k_replacer.h 顶部的规则说明，
// 再动手实现；test/buffer/lru_k_replacer_test.cpp 是你的判卷老师。

bool LRUKReplacer::Evict(frame_id_t *frame_id) {
    (void)frame_id;
    throw NotImplementedException("Lab1: implement LRUKReplacer::Evict (docs/lab1-buffer-pool.md)");
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
    (void)frame_id;
    throw NotImplementedException("Lab1: implement LRUKReplacer::RecordAccess");
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    (void)frame_id;
    (void)set_evictable;
    throw NotImplementedException("Lab1: implement LRUKReplacer::SetEvictable");
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
    (void)frame_id;
    throw NotImplementedException("Lab1: implement LRUKReplacer::Remove");
}

size_t LRUKReplacer::Size() const {
    throw NotImplementedException("Lab1: implement LRUKReplacer::Size");
}

}  // namespace hdb
