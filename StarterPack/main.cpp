#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>
#include "bplus_tree_disk.h"

using namespace std;
using namespace std::chrono;

FILE* db_file = nullptr;
DBHeader header;

// Disk I/O counters and helper functions:
int disk_read_count = 0;
int disk_write_count = 0;

void resetIOCounters();

// TODO for students: Implement page read/write, search, insert, and query functions...
void readNode(int offset, BPlusNode& node);
void writeNode(int offset, BPlusNode& node);
int linearSearch(const int keys[], int num_keys, int target);
int binarySearch(const int keys[], int num_keys, int target);
bool insertRecursive(int current_offset, int id, const char* payload, int& new_key, int& new_offset);
void insertRecord(int id, const char* payload);
bool pointQueryBPlusStyle(int target_id, bool use_binary_search);
bool pointQueryBTreeStyle(int target_id, bool use_binary_search);
int rangeQueryBPlusStyle(int start_id, int end_id, bool use_binary_search);
int rangeQueryBTreeStyle(int start_id, int end_id, bool use_binary_search);

void runBenchmark(int N, const int query_ids[]) {
    cout << "\n========== BENCHMARK WITH N = " << N << " ==========\n";
    
    int range_size = (N < 2000) ? 500 : 1000;

    long long total_time = 0;
    long long total_reads = 0;

    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBTreeStyle(query_ids[i], false);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Point Query | B-Tree Style  | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBTreeStyle(query_ids[i], true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Point Query | B-Tree Style  | Binary Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBPlusStyle(query_ids[i], false);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Point Query | B+ Tree Style | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBPlusStyle(query_ids[i], true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Point Query | B+ Tree Style | Binary Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        rangeQueryBTreeStyle(query_ids[i], query_ids[i] + range_size, false);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Range Query | B-Tree Style  | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        rangeQueryBTreeStyle(query_ids[i], query_ids[i] + range_size, true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Range Query | B-Tree Style  | Binary Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        rangeQueryBPlusStyle(query_ids[i], query_ids[i] + range_size, false);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Range Query | B+ Tree Style | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        rangeQueryBPlusStyle(query_ids[i], query_ids[i] + range_size, true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += disk_read_count;
    }
    cout << "Range Query | B+ Tree Style | Binary Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";
}

int main() {
    // Preload the first 100 IDs from dataset_large.csv to serve as benchmark target IDs
    int query_ids[100];
    ifstream csv_setup_file("dataset_large.csv");
    if (!csv_setup_file.is_open()) {
        cerr << "Error: Could not find dataset_large.csv. Run data_generator.cpp first!\n";
        return 1;
    }
    string setup_line;
    getline(csv_setup_file, setup_line); // Skip header row
    int query_count = 0;
    while (query_count < 100 && getline(csv_setup_file, setup_line)) {
        stringstream ss(setup_line);
        string id_str;
        getline(ss, id_str, ',');
        query_ids[query_count++] = stoi(id_str);
    }
    csv_setup_file.close();

    if (query_count < 100) {
        cerr << "Error: dataset_large.csv does not contain enough records (100 required) for benchmarking!\n";
        return 1;
    }

    const int DATA_SIZES[] = {1000, 3000, 10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    const int NUM_SIZES = sizeof(DATA_SIZES) / sizeof(DATA_SIZES[0]);

    for (int i = 0; i < NUM_SIZES; ++i) {
        int N = DATA_SIZES[i];
        string filename = "index_" + to_string(N) + ".dat";
        
        db_file = fopen(filename.c_str(), "r+b");
        if (!db_file) {
            db_file = fopen(filename.c_str(), "w+b");
            header.root_offset = -1;
            header.total_nodes = 0;
            header.free_list_offset = -1;
            fseek(db_file, 0, SEEK_SET);
            fwrite(&header, sizeof(DBHeader), 1, db_file);
        } else {
            fseek(db_file, 0, SEEK_SET);
            fread(&header, sizeof(DBHeader), 1, db_file);
        }

        if (header.root_offset == -1) {
            cout << "Importing " << N << " records...\n";
            ifstream csv_file("dataset_large.csv");
            if (!csv_file.is_open()) {
                cerr << "Error: Could not find dataset_large.csv!\n";
                fclose(db_file);
                return 1;
            }

            string line;
            getline(csv_file, line); // Skip header row

            int count = 0;
            while (count < N && getline(csv_file, line)) {
                stringstream ss(line);
                string id_str, payload;
                getline(ss, id_str, ',');
                getline(ss, payload, ',');
                insertRecord(stoi(id_str), payload.c_str());
                count++;
            }
            csv_file.close();
        }

        runBenchmark(N, query_ids);

        fclose(db_file);
        db_file = nullptr;
    }
    return 0;
}

void resetIOCounters() {
    disk_read_count = 0;
    disk_write_count = 0;
}
void readNode(int offset, BPlusNode& node) {
    if (!db_file) return; 
    fseek(db_file, offset, SEEK_SET); 
    fread(&node, sizeof(BPlusNode), 1, db_file); 
    disk_read_count++; 
}
void writeNode(int offset, BPlusNode& node) {
    if (!db_file) return; 
    fseek(db_file, offset, SEEK_SET); 
    fwrite(&node, sizeof(BPlusNode), 1, db_file); 
    disk_write_count++; 
}
int allocateNode() {
    int offset = sizeof(DBHeader) + header.total_nodes* sizeof(BPlusNode); 
    header.total_nodes++; 
    fseek(db_file, 0, SEEK_SET); 
    fwrite(&header, sizeof(DBHeader), 1, db_file);
    BPlusNode node; 
    memset(&node, 0, sizeof(BPlusNode)); 
    fseek(db_file, offset, SEEK_SET); 
    fwrite(&node, sizeof(BPlusNode), 1, db_file); 
    return offset; 
}
int linearSearch(const int keys[], int num_keys, int target) {
    for (int i = 0; i < num_keys; ++i) {
        if (keys[i] >= target) {
            return i;
        }
    }
    return num_keys;
}

int binarySearch(const int keys[], int num_keys, int target) {
    int left = 0, right = num_keys - 1;
    int ans = num_keys;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (keys[mid] >= target) {
            ans = mid;        // Ghi nhận vị trí tiềm năng
            right = mid - 1;  // Tiếp tục tìm về bên trái
        } else {
            left = mid + 1;
        }
    }
    return ans;
}

int linearSearch(const Record records[], int num_records, int target) {
    for (int i = 0; i < num_records; ++i) {
        if (records[i].id >= target) {
            return i;
        }
    }
    return num_records;
}