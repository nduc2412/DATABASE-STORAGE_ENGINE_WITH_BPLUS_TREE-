#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "bplus_tree_disk.h"

// Các hàm đọc/ghi Node trực tiếp với ổ cứng
void readNode(int offset, BPlusNode& node);
void writeNode(int offset, BPlusNode& node);
int allocateNode();

// Các hàm thao tác trên B+ Tree
bool insertRecursive(int current_offset, int id, const char* payload, int& new_key, int& new_offset);
void insertRecord(int id, const char* payload);

bool pointQueryBPlusStyle(int target_id, bool use_binary_search);
int rangeQueryBPlusStyle(int start_id, int end_id, bool use_binary_search);

#endif // BPLUS_TREE_H
