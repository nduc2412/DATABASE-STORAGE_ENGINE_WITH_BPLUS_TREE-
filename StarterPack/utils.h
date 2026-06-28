#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include "BPlusTree/bplus_tree_disk.h"
#include "BTree/btree_disk.h"

// Các con trỏ file và Header toàn cục của cơ sở dữ liệu
extern FILE* db_file;
extern DBHeader header;

extern FILE* btree_db_file;
extern DBHeader btree_header;

// Biến đếm số lượt I/O (Disk Reads/Writes)
extern int disk_read_count;
extern int disk_write_count;
extern int btree_read_count;
extern int btree_write_count;

// Đặt lại các biến đếm về 0 trước khi bắt đầu đo lường
void resetIOCounters();

// Các hàm hỗ trợ tìm kiếm trên mảng trong bộ nhớ (In-memory search)
int linearSearch(const int keys[], int num_keys, int target);
int binarySearch(const int keys[], int num_keys, int target);
int linearSearch(const Record records[], int num_records, int target);
int binarySearch(const Record records[], int num_records, int target);

#endif // UTILS_H
