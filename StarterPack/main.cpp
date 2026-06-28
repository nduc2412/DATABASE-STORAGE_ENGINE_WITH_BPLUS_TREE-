#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "utils.h"
#include "bplus_tree.h"
#include "btree.h"

using namespace std;
using namespace std::chrono;

void runBenchmark(int N, const int query_ids[]) {
    cout << "\n========== BENCHMARK WITH N = " << N << " ==========" << endl;
    
    int range_size = (N < 2000) ? 500 : 1000;

    long long total_time = 0;
    long long total_reads = 0;

    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBTreeStyle(query_ids[i], false);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += btree_read_count;
    }
    cout << "Point Query | B-Tree Style  | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        pointQueryBTreeStyle(query_ids[i], true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += btree_read_count;
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
        total_reads += btree_read_count;
    }
    cout << "Range Query | B-Tree Style  | Linear Search : " << total_time / 100 << " ns | " << total_reads / 100.0 << " reads\n";

    total_time = 0; total_reads = 0;
    for (int i = 0; i < 100; ++i) {
        resetIOCounters();
        auto start = high_resolution_clock::now();
        rangeQueryBTreeStyle(query_ids[i], query_ids[i] + range_size, true);
        auto end = high_resolution_clock::now();
        total_time += duration_cast<nanoseconds>(end - start).count();
        total_reads += btree_read_count;
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
    int query_ids[100];
    ifstream csv_setup_file("dataset_large.csv");
    if (!csv_setup_file.is_open()) {
        cerr << "Error: Could not find dataset_large.csv. Run data_generator.cpp first!\n";
        return 1;
    }
    string setup_line;
    getline(csv_setup_file, setup_line); 
    int query_count = 0;
    while (query_count < 100 && getline(csv_setup_file, setup_line)) {
        stringstream ss(setup_line);
        string id_str;
        getline(ss, id_str, ',');
        query_ids[query_count++] = stoi(id_str);
    }
    csv_setup_file.close();

    if (query_count < 100) {
        cerr << "Error: dataset_large.csv does not contain enough records for benchmarking!\n";
        return 1;
    }

    const int DATA_SIZES[] = {1000, 3000, 10000, 30000, 100000, 300000, 1000000, 3000000, 10000000};
    const int NUM_SIZES = sizeof(DATA_SIZES) / sizeof(DATA_SIZES[0]);

    for (int i = 0; i < NUM_SIZES; ++i) {
        int N = DATA_SIZES[i];
        string filename = "index_" + to_string(N) + ".dat";
        string btree_filename = "btree_index_" + to_string(N) + ".dat";
        
        db_file = fopen(filename.c_str(), "r+b");
        bool bplus_needs_import = false;
        if (!db_file) {
            db_file = fopen(filename.c_str(), "w+b");
            header.root_offset = -1;
            header.total_nodes = 0;
            header.free_list_offset = -1;
            fseek(db_file, 0, SEEK_SET);
            fwrite(&header, sizeof(DBHeader), 1, db_file);
            bplus_needs_import = true;
        } else {
            fseek(db_file, 0, SEEK_SET);
            fread(&header, sizeof(DBHeader), 1, db_file);
        }

        btree_db_file = fopen(btree_filename.c_str(), "r+b");
        bool btree_needs_import = false;
        if (!btree_db_file) {
            btree_db_file = fopen(btree_filename.c_str(), "w+b");
            btree_header.root_offset = -1;
            btree_header.total_nodes = 0;
            btree_header.free_list_offset = -1;
            fseek(btree_db_file, 0, SEEK_SET);
            fwrite(&btree_header, sizeof(DBHeader), 1, btree_db_file);
            btree_needs_import = true;
        } else {
            fseek(btree_db_file, 0, SEEK_SET);
            fread(&btree_header, sizeof(DBHeader), 1, btree_db_file);
        }

        if (bplus_needs_import || btree_needs_import) {
            cout << "Importing " << N << " records..." << endl;
            ifstream csv_file("dataset_large.csv");
            if (!csv_file.is_open()) {
                cerr << "Error: Could not find dataset_large.csv!\n";
                if (db_file) fclose(db_file);
                if (btree_db_file) fclose(btree_db_file);
                return 1;
            }

            string line;
            getline(csv_file, line); 

            int count = 0;
            while (count < N && getline(csv_file, line)) {
                stringstream ss(line);
                string id_str, payload;
                getline(ss, id_str, ',');
                getline(ss, payload, ',');
                int id = stoi(id_str);
                
                if (bplus_needs_import) insertRecord(id, payload.c_str());
                if (btree_needs_import) insertBTreeRecord(id, payload.c_str());
                count++;
            }
            csv_file.close();
        }

        runBenchmark(N, query_ids);

        if (db_file) {
            fclose(db_file);
            db_file = nullptr;
        }
        if (btree_db_file) {
            fclose(btree_db_file);
            btree_db_file = nullptr;
        }
    }
    return 0;
}
