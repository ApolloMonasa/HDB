#include "hdb/storage/disk/disk_manager.h"

#include <cassert>
#include <cstring>
#include <unistd.h>

#include "hdb/common/exception.h"
#include "hdb/common/logger.h"

namespace hdb {

namespace {
/// 元数据页里前 8 字节的固定布局
struct DiskMetadata {
    int32_t num_pages;
    int32_t free_head;
};
static_assert(sizeof(DiskMetadata) == 8, "metadata layout");

/// 页号 -> 文件偏移（页 0 是元数据页，数据页从 1 开始）
inline long PageOffset(page_id_t page_id) { return static_cast<long>(page_id) * PAGE_SIZE; }
}  // namespace

DiskManager::DiskManager(const std::string &db_file) : file_name_(db_file) {
    db_io_ = std::fopen(db_file.c_str(), "r+b");
    if (db_io_ == nullptr) {
        // 文件不存在：以"w+b"新建（读写、二进制）
        db_io_ = std::fopen(db_file.c_str(), "w+b");
        if (db_io_ == nullptr) {
            throw IOException("cannot open or create database file: " + db_file);
        }
        // 全新文件：写入初始元数据（只有元数据页，无空闲页）
        num_pages_ = 1;
        free_head_ = INVALID_PAGE_ID;
        WriteMetadata();
        HDB_LOG_INFO("created new database file %s", db_file.c_str());
        return;
    }
    ReadMetadataOrFormat();
    HDB_LOG_INFO("opened database file %s (pages=%d)", db_file.c_str(), num_pages_);
}

DiskManager::~DiskManager() {
    if (db_io_ != nullptr) {
        try {
            WriteMetadata();
        } catch (const HDBException &e) {
            HDB_LOG_ERROR("failed to flush metadata on shutdown: %s", e.what());
        }
        std::fclose(db_io_);
        db_io_ = nullptr;
    }
}

void DiskManager::ReadMetadataOrFormat() {
    DiskMetadata meta{};
    if (std::fseek(db_io_, PageOffset(0), SEEK_SET) != 0 ||
        std::fread(&meta, sizeof(meta), 1, db_io_) != 1) {
        // 空文件（比如 touch 出来的）：当作全新数据库格式化
        num_pages_ = 1;
        free_head_ = INVALID_PAGE_ID;
        WriteMetadata();
        return;
    }
    if (meta.num_pages < 1) {
        throw IOException("corrupted metadata in " + file_name_);
    }
    num_pages_ = meta.num_pages;
    free_head_ = meta.free_head;
}

void DiskManager::WriteMetadata() {
    DiskMetadata meta{num_pages_, free_head_};
    if (std::fseek(db_io_, PageOffset(0), SEEK_SET) != 0 ||
        std::fwrite(&meta, sizeof(meta), 1, db_io_) != 1 || std::fflush(db_io_) != 0) {
        throw IOException("failed to write metadata in " + file_name_);
    }
}

void DiskManager::ReadPage(page_id_t page_id, char *page_data) {
    if (page_id <= 0 || page_id >= num_pages_) {
        throw IOException("ReadPage: invalid page id " + std::to_string(page_id) + " in " +
                          file_name_);
    }
    if (std::fseek(db_io_, PageOffset(page_id), SEEK_SET) != 0 ||
        std::fread(page_data, PAGE_SIZE, 1, db_io_) != 1) {
        io_failed_ = true;
        throw IOException("ReadPage: fread failed on page " + std::to_string(page_id));
    }
}

void DiskManager::WritePage(page_id_t page_id, const char *page_data) {
    if (page_id <= 0 || page_id >= num_pages_) {
        throw IOException("WritePage: invalid page id " + std::to_string(page_id) + " in " +
                          file_name_);
    }
    if (std::fseek(db_io_, PageOffset(page_id), SEEK_SET) != 0 ||
        std::fwrite(page_data, PAGE_SIZE, 1, db_io_) != 1 || std::fflush(db_io_) != 0) {
        io_failed_ = true;
        throw IOException("WritePage: fwrite failed on page " + std::to_string(page_id));
    }
}

page_id_t DiskManager::IncreaseNumPages() {
    if (free_head_ != INVALID_PAGE_ID) {
        // 复用空闲链表上的页号：读出它的 next 指针，头弹出链表
        page_id_t reused = free_head_;
        int32_t next = INVALID_PAGE_ID;
        if (std::fseek(db_io_, PageOffset(reused), SEEK_SET) == 0) {
            // 只读前 4 字节；即使读失败也当作链表末端，不阻塞分配
            std::fread(&next, sizeof(next), 1, db_io_);
        }
        free_head_ = next;
        WriteMetadata();
        return reused;
    }
    page_id_t new_id = num_pages_;
    num_pages_++;
    WriteMetadata();
    return new_id;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
    if (page_id <= 0 || page_id >= num_pages_) {
        throw IOException("DeallocatePage: invalid page id " + std::to_string(page_id));
    }
    // 头插空闲链表：本页前 4 字节存原链表头
    int32_t old_head = free_head_;
    if (std::fseek(db_io_, PageOffset(page_id), SEEK_SET) != 0 ||
        std::fwrite(&old_head, sizeof(old_head), 1, db_io_) != 1) {
        throw IOException("DeallocatePage: fwrite failed on page " + std::to_string(page_id));
    }
    std::fflush(db_io_);
    free_head_ = page_id;
    WriteMetadata();
}

}  // namespace hdb
