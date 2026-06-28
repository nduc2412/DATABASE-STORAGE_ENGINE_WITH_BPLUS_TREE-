#ifndef BPLUS_TREE_DISK_H
#define BPLUS_TREE_DISK_H

#include <iostream>

#define PAGE_SIZE 4096
#define PAYLOAD_SIZE 60 // Simulated data string size, including '\0' character

/* =========================================================
   CALCULATING NODE CAPACITY TO NOT EXCEED 4096 BYTES
   ========================================================= */
// Common fixed size: 
// is_leaf (1 byte) + bool padding (3 bytes) + num_keys (4 bytes) = 8 bytes.
// Remaining space for Union: 4096 - 8 = 4088 bytes.

// 1. FOR LEAF NODES
// Each Record: ID (4 bytes) + Payload (60 bytes) = 64 bytes.
// Leaf Node also needs 1 pointer next_leaf_offset (4 bytes).
// Equation: 8 + 64 * MAX_LEAF_KEYS + 4 <= 4096  => MAX_LEAF_KEYS <= 63.8
// We choose MAX_LEAF_KEYS = 60 for safety, leaving a small buffer.
#define MAX_LEAF_KEYS 60

// 2. FOR INTERNAL NODES
// Contains only Keys (4 bytes) and Child_Offsets (4 bytes).
// Equation: 8 + 4 * MAX_INTERNAL_KEYS + 4 * (MAX_INTERNAL_KEYS + 1) <= 4096 
// => 8 * MAX_INTERNAL_KEYS + 12 <= 4096 
// => MAX_INTERNAL_KEYS <= 510.5
// WARNING: MAX_INTERNAL_KEYS MUST NOT exceed 510. If set to 511, sizeof(BPlusNode) 
// becomes 4100 bytes (> 4096), causing silent page overlap/corruption on disk.
#define MAX_INTERNAL_KEYS 510

/* ========================================================= */

// Structure for a single row of data (Record) stored in Leaf Nodes
struct Record {
    int id;
    char payload[PAYLOAD_SIZE];
};

// Standard B+ Tree Node structure on disk
struct BPlusNode {
    bool is_leaf;       // true if Leaf, false if Internal Node
    char padding1[3];   // Padding bytes for memory alignment (prevents bugs across different compilers/OSs)
    int num_keys;       // Current number of keys contained

    // Using UNION: Nested structures share the same physical memory space.
    // The union size equals the size of the largest nested structure.
    // If is_leaf = true, records array is used. If is_leaf = false, internal struct is used.
    union {
        // Structure for Internal Nodes (DOES NOT contain payload)
        struct {
            int keys[MAX_INTERNAL_KEYS];
            int children_offsets[MAX_INTERNAL_KEYS + 1];
        } internal;

        // Structure for Leaf Nodes (CONTAINS Payload and next_leaf pointer)
        struct {
            Record records[MAX_LEAF_KEYS];
            int next_leaf_offset; // Disk offset to the next Leaf Node (supports Range Query)
        } leaf;
    };
};

// Header structure located at the very START of index.dat file (offset = 0)
// This struct is padded to exactly 1 PAGE_SIZE (4096 bytes) so that subsequent nodes
// align perfectly with disk sectors when offsets are multiplied by 4096.
struct DBHeader {
    int root_offset;       // Byte offset of the current Root Node
    int total_nodes;       // Total number of nodes created
    int free_list_offset;  // Disk offset of the first reusable empty page (-1 if none)
    
    // Padding to fill exactly 4KB (4096 bytes - 3 int variables * 4 bytes = 4084 bytes)
    char reserved[PAGE_SIZE - 12]; 
};

#endif // BPLUS_TREE_DISK_H
