# LAB 1: DATABASE STORAGE ENGINE WITH ON-DISK B+ TREE

## 1. Real-world Problem

In relational database management systems (such as MySQL or PostgreSQL), the amount of data often exceeds the capacity of RAM. Therefore, data must be stored directly on external storage (HDD/SSD) in the form of fixed-size blocks called Pages, typically 4KB in size. However, a problem arises: disk access speed is tens of thousands of times slower than RAM. When a system faces massive data, the disk I/O cost immediately becomes the bottleneck determining the speed of the entire system, and this weakness is even more prominent when processing the most common operations in practice.

Examples of Range Query:

```sql
SELECT * FROM transactions WHERE date BETWEEN '2026-01-01' AND '2026-12-31';
SELECT * FROM users WHERE score >= 80 AND score <= 100;
```

Although you have been equipped with knowledge about B-Trees in this course, in industrial practice, 99% of database engines choose B+ Trees as their core storage structure rather than B-Trees. To understand why this preference exists, this project will help you put yourself in the shoes of a Systems Engineer. Your task is to build a complete database storage engine from scratch using a B+ Tree operating directly on disk. Through benchmarking and pitting the B+ Tree directly against the B-Tree, you will gain practical insights and analyze the outstanding advantages that the B+ Tree architecture brings.

## 2. Querying Algorithms

You will implement and compare 2 querying approaches on the Tree:
- **B-Tree Style Search**: The primitive approach, performing a recursive tree traversal from top to bottom to retrieve each element satisfying the condition.
- **B+ Tree Style Search**: Taking advantage of the Linked List connecting the Leaf Nodes at the bottom level to scan data linearly, radically optimizing Disk I/O.

Additionally, when performing internal search inside a Node (Internal Search), you will also examine the performance between 2 strategies:
- **Linear Search**: Linear search on the key array.
- **Binary Search**: Binary search on the sorted key array.

## 3. Provided Data Structures (Input Data)

**Data Source (CSV File)**
- Students need to compile and run the `data_generator.cpp` file to generate a sample dataset as a `dataset_large.csv` file. This CSV file contains raw data rows, where each row is a set of information comprising a random integer (ID) and a random string.

**Record Concept**
- Each data row from the CSV file, when read into memory, will be stored under the structure of a `Record`. A `Record` is designed with an exact total size of 64 bytes, including an `id` (4-byte integer) and a `payload` (a 60-byte character array used to store a garbage string).

**On-disk Node Structure (`bplus_tree_disk.h`)**
You are provided with the `bplus_tree_disk.h` file containing the physical structure definitions:
- **Node Structure (`BPlusNode`)**: Uses the C/C++ `union` keyword to force the total size of the Node to exactly match the standard Page size of 4KB (4096 bytes). This technique allows a 4KB memory block to flexibly act as either a Leaf Node or an Internal Node depending on the `is_leaf` flag. **Strictly do not modify this file**.
- **Node Capacity**: Based on memory alignment calculations, with a `Record` size of 64 bytes, a Leaf Node can hold a maximum of 60 Records. In contrast, an Internal Node does not store the `payload` but only stores Keys (4-byte integers) and pointer arrays (disk offsets), so it can hold a maximum of 510 Keys.

## 4. Implementation Steps & Technical Requirements

**Step 1: File Management (File I/O)**
- First, compile and run `data_generator.cpp` to create the sample dataset `dataset_large.csv`.
- Read the CSV file to build the tree structure and save it in a binary file format `index.dat`. We choose to create a `.dat` binary file instead of manipulating the CSV because the binary file helps divide memory into exact 4KB fixed blocks (Pages), perfectly simulating how physical memory of the operating system and hard drive works.
- Implement hard drive interface functions (here, for the `.dat` file, using C++ functions like `fseek`, `fread`, `fwrite`):
  - `readNode(int offset, BPlusNode& node)`: Takes the byte `offset`, reads exactly 4KB of data from the hard drive, and copies it into the `node` variable in RAM.
  - `writeNode(int offset, BPlusNode& node)`: Takes 4KB of data from the `node` variable and overwrites the hard drive at the `offset` position.
  - Write a Node allocation helper function to find the next empty position (end of file) used for allocating a new Node.

**Step 2: Core B+ Tree Implementation (Insert)**
- Implement the algorithm to insert data into the B+ Tree. The function receives the ID and the payload string, traversing down to the leaf node to insert.
- **Core technical requirement**: Thoroughly handle the Split mechanism:
  - **Leaf Node Split**: When a leaf is full, split the elements in half, copy the median Key to the parent Node (but retain that Key at the leaf), and reset the `next_leaf_offset` pointer to maintain the Linked List.
  - **Internal Node Split**: When an internal Node is full, split the elements in half, but push the median Key straight up to the parent (the current Node loses that Key).

**Step 3: Implement Query Engines**
- Implement 4 query functions:
  - `pointQueryBTree` and `pointQueryBPlus`: Take the target ID, returning a boolean (whether it exists or not).
  - `rangeQueryBTree` and `rangeQueryBPlus`: Take the range `[L, R]`, and count the number of Records within this range.
- The query functions must accept a flag parameter to switch back and forth between performing Linear Search or Binary Search inside each Node.

## 5. Rules & Constraints

- **Ban on loading the entire tree into RAM**: The maximum RAM allowed is 16MB. Strictly adhere to the principle: Only read individual Nodes from the hard drive when needed, and write them back if modified.
- **Ban on modifying memory structure**: Absolutely do not use dynamic memory pointers (`Node*`) to link the tree. You must use the byte position in the file (`diskOffsets` of type `int`) as "file pointers".
- **No dynamic data structures**: Do not use `std::vector` or `std::map` inside a Node's structure.

## 6. Assumptions & Clarifications

- **Fixed Size**: Each Record has a size of 64 bytes (4 bytes ID + 60 bytes garbage). Therefore, a 4KB Page design will contain a maximum of 60 Records at leaves and 510 Keys at internal nodes.
- **Time & Disk I/O Measurement**:
  - Use a global counter variable `disk_read_count` to count how many times the `fread` function is called (number of disk accesses).
  - Use the C++ `<chrono>` library (milliseconds/nanoseconds) for timing.
- **OS Page Cache**: The Operating System automatically buffers files into RAM (OS Page Cache). Therefore, students must **only time the execution around the Query operations**. Absolutely do not include the CSV file reading time or the initial Tree Load/Insert time in the query benchmark results.

## 7. Output & Submission Requirements

### 7.1. Empirical Benchmark
Students measure the average results of 100 random query runs to ensure objectivity. Fill in the results using the format: `Mean Time (ns) | Mean Disk Reads` (Example: `18,400 ns | 3.0 reads`) in the 2 tables below.

**Table 1: Point Query Performance Comparison**

| Data Size ($N$) | B-Tree Style (Linear Search) | B-Tree Style (Binary Search) | B+ Tree Style (Linear Search) | B+ Tree Style (Binary Search) |
| :--------------| :--------------------------:| :--------------------------:| :----------------------------:| :----------------------------:|
| **1,000**      | /                           | /                           | /                             | /                             |
| **3,000**      | /                           | /                           | /                             | /                             |
| **10,000**     | /                           | /                           | /                             | /                             |
| **30,000**     | /                           | /                           | /                             | /                             |
| **100,000**    | /                           | /                           | /                             | /                             |
| **300,000**    | /                           | /                           | /                             | /                             |
| **1,000,000**  | /                           | /                           | /                             | /                             |
| **3,000,000**  | /                           | /                           | /                             | /                             |
| **10,000,000** | /                           | /                           | /                             | /                             |

**Table 2: Range Query Performance Comparison**
*(Range Size: $R = 500$ records for $N = 1000$, and $R = 1000$ records for the other $N$ levels)*

| Data Size ($N$) | B-Tree Style (Linear Search) | B-Tree Style (Binary Search) | B+ Tree Style (Linear Search) | B+ Tree Style (Binary Search) |
| :--------------| :--------------------------:| :--------------------------:| :----------------------------:| :----------------------------:|
| **1,000**      | /                           | /                           | /                             | /                             |
| **3,000**      | /                           | /                           | /                             | /                             |
| **10,000**     | /                           | /                           | /                             | /                             |
| **30,000**     | /                           | /                           | /                             | /                             |
| **100,000**    | /                           | /                           | /                             | /                             |
| **300,000**    | /                           | /                           | /                             | /                             |
| **1,000,000**  | /                           | /                           | /                             | /                             |
| **3,000,000**  | /                           | /                           | /                             | /                             |
| **10,000,000** | /                           | /                           | /                             | /                             |

### 7.2. Submission Format
Create a compressed file in `.zip` format with the naming syntax: `[StudentID1]_[StudentID2]_Lab1.zip`.

Submission directory structure:
```plaintext
[StudentID1]_[StudentID2]_Lab1/
│
├── Solution/
│   ├── main.cpp
│   ├── bplus_tree_disk.h
│   └── (other .cpp/.h files defined by you)
│
└── Report.pdf
```

> [!WARNING]
> **Warning**: Do not include executable files (`.exe`, `.o`), build directories (`.vs`, `cmake`), and absolutely **DO NOT** attach the data files.
- **Strictly do not submit the `index.dat` and `dataset_large.csv` files** due to their massive sizes. Violations may result in score deductions.

### 7.3. PDF Report & Discussion Guidelines
Plot at least 2 Log-scale charts (for both axes) illustrating the results. Based on the empirical data, answer the following 4 advanced topics:

#### Topic 1: Range Query Performance & Disk I/O Bottleneck
* **Analysis Guidelines:**
  - Compare and explain the massive discrepancy in disk read counts (Disk Reads) and execution time (nanoseconds) between the B+ Tree style and B-Tree style as $N$ scales up.
  - Analyze the core role of the `next_leaf_offset` field (linked list at leaves) in optimizing Disk I/O (converting from Random I/O to Sequential I/O).

#### Topic 2: B+ Tree Height & Node Structure Optimization
* **Analysis Guidelines:**
  - Analyze the growth pattern of the tree height relative to the data size $N$ based on the Point Query Disk Reads. Does the tree height grow linearly, logarithmically, or in some other form?
  - Analyze the crucial importance of a large Branching Factor in keeping the on-disk tree shallow (low height).

#### Topic 3: CPU-Bound and I/O-Bound Interaction
* **Analysis Guidelines:**
  - Compare the performance of Linear Search and Binary Search inside a node. Which factor changes most noticeably in the measured results (CPU time or Disk Reads)? Explain why.
  - Evaluate the impact of the internal search algorithm (CPU) if the actual Page size configuration were larger (e.g., 16KB or 32KB).

#### Topic 4: Data Fragmentation & System Reality
* **Analysis Guidelines:**
  - Analyze the phenomenon: When inserting millions of random records, leaf nodes continuously Split, causing logically adjacent nodes (via `next_leaf`) to be physically scattered across the `.dat` file.
  - Analyze the impact of this phenomenon on the read/write speed of physical storage devices (comparing mechanical HDDs and SSDs).
  - *Extension:* Propose or research real-world solutions used by modern DBMSs (like MySQL InnoDB, PostgreSQL) to minimize fragmentation or optimize I/O (e.g., defragmentation, buffering/caching, LSM-Tree structure, etc.).
