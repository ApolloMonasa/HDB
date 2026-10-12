#include "hdb/concurrency/mvcc.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab6（选做）填空位。接口语义见 mvcc.h 顶部注释。

uint64_t MvccManager::BeginReadTimestamp() {
    throw NotImplementedException("Lab6: implement MvccManager::BeginReadTimestamp (docs/lab6-mvcc.md)");
}

uint64_t MvccManager::CommitTimestamp() {
    throw NotImplementedException("Lab6: implement MvccManager::CommitTimestamp");
}

bool MvccManager::ReconstructTuple(uint64_t read_ts, const std::vector<VersionRecord> &versions,
                                   Tuple *out) {
    (void)read_ts;
    (void)versions;
    (void)out;
    throw NotImplementedException("Lab6: implement MvccManager::ReconstructTuple");
}

bool MvccManager::IsVisible(uint64_t read_ts, uint64_t version_ts, bool is_deleted) {
    (void)read_ts;
    (void)version_ts;
    (void)is_deleted;
    throw NotImplementedException("Lab6: implement MvccManager::IsVisible");
}

}  // namespace hdb
