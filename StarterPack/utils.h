#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include "BPlusTree/bplus_tree_disk.h"
#include "BTree/btree_disk.h"

// Global Database Pointers and Headers
extern FILE* db_file;
extern DBHeader header;

extern FILE* btree_db_file;
extern DBHeader btree_header;

// I/O Counters
extern int disk_read_count;
extern int disk_write_count;
extern int btree_read_count;
extern int btree_write_count;

void resetIOCounters();

// Search Helpers
int linearSearch(const int keys[], int num_keys, int target);
int binarySearch(const int keys[], int num_keys, int target);
int linearSearch(const Record records[], int num_records, int target);
int binarySearch(const Record records[], int num_records, int target);

#endif // UTILS_H
