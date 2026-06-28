#ifndef BTREE_H
#define BTREE_H

#include "btree_disk.h"

// Các hàm thao tác đọc/ghi Node của True B-Tree với đĩa cứng
void readBNode(int offset, BNode& node);
void writeBNode(int offset, BNode& node);
int allocateBNode();

// Các hàm hỗ trợ thêm mới dữ liệu vào True B-Tree
bool insertBTreeRecursive(int current_offset, int id, const char* payload, Record& promoted_record, int& new_offset);
void insertBTreeRecord(int id, const char* payload);

// Các hàm tìm kiếm dữ liệu trên True B-Tree
bool pointQueryBTreeStyle(int target_id, bool use_binary_search);
int rangeQueryBTreeRecursive(int offset, int start_id, int end_id, bool use_binary_search);
int rangeQueryBTreeStyle(int start_id, int end_id, bool use_binary_search);

#endif // BTREE_H
