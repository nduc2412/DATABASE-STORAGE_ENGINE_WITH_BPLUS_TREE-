#include "utils.h"

FILE* db_file = nullptr;
DBHeader header;

FILE* btree_db_file = nullptr;
DBHeader btree_header;

int disk_read_count = 0;
int disk_write_count = 0;
int btree_read_count = 0;
int btree_write_count = 0;

void resetIOCounters() {
    disk_read_count = 0;
    disk_write_count = 0;
    btree_read_count = 0;
    btree_write_count = 0;
}

int linearSearch(const int keys[], int num_keys, int target) {
    for (int i = 0; i < num_keys; ++i) {
        if (keys[i] >= target) return i;
    }
    return num_keys;
}

int binarySearch(const int keys[], int num_keys, int target) {
    int left = 0, right = num_keys - 1;
    int ans = num_keys;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (keys[mid] >= target) {
            ans = mid;
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return ans;
}

int linearSearch(const Record records[], int num_records, int target) {
    for (int i = 0; i < num_records; ++i) {
        if (records[i].id >= target) return i;
    }
    return num_records;
}

int binarySearch(const Record records[], int num_records, int target) {
    int left = 0, right = num_records - 1;
    int ans = num_records;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (records[mid].id >= target) {
            ans = mid;
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return ans;
}
