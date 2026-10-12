#pragma once

#include <cstddef>

#include "hdb/common/config.h"

namespace hdb {

/**
 * @brief 页面替换策略的抽象接口（框架给定）。
 *
 * 缓冲池满了以后"牺牲谁"是一个可插拔的决策：
 * LRU、Clock、LRU-K、ARC……接口只有四个动作——
 * 记录访问、标记可否驱逐、驱逐、查询规模。
 * Lab1 你实现 LRU-K；换策略不需要动缓冲池一行代码。
 */
class Replacer {
  public:
    virtual ~Replacer() = default;

    /**
     * 从可驱逐帧里挑一个牺牲者。
     * @param frame_id 输出被选中的帧号
     * @return false 表示当前没有可驱逐帧
     */
    virtual bool Evict(frame_id_t *frame_id) = 0;

    /** 帧被访问（页被 pin 进该帧）时记录一次历史 */
    virtual void RecordAccess(frame_id_t frame_id) = 0;

    /**
     * 设置帧是否可被驱逐。
     * @note pin 计数为 0 的页 → 可驱逐；计数 > 0 → 不可驱逐。
     *       只有"被记为可驱逐"的帧才参与 Evict。
     */
    virtual void SetEvictable(frame_id_t frame_id, bool set_evictable) = 0;

    /** 从替换器中彻底移除一帧（页面被删除时调用） */
    virtual void Remove(frame_id_t frame_id) = 0;

    /** 当前可驱逐帧的数量 */
    virtual size_t Size() const = 0;
};

}  // namespace hdb
