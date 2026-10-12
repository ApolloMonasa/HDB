#include "hdb/common/rid.h"

#include <ostream>

namespace hdb {

std::ostream &operator<<(std::ostream &os, const RID &rid) {
    os << "(" << rid.page_id_ << "," << rid.slot_id_ << ")";
    return os;
}

}  // namespace hdb
