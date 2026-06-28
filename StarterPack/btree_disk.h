#ifndef BTREE_DISK_H
#define BTREE_DISK_H

#include "bplus_tree_disk.h"

#define MAX_BTREE_KEYS 60

// Standard B-Tree Node structure on disk
struct BNode {
    bool is_leaf;       // true if Leaf, false if Internal Node
    char padding1[3];   // Padding bytes for memory alignment
    int num_keys;       // Current number of records contained

    // In a B-Tree, both internal and leaf nodes contain Records 
    Record records[MAX_BTREE_KEYS];
    
    // Child offsets for routing
    int children_offsets[MAX_BTREE_KEYS + 1];

    // Padding to fill exactly 4KB (4096 bytes)
    // 8 + 60*64 + 61*4 = 4092 bytes. Needs 4 bytes padding.
    char reserved[4];
};

#endif // BTREE_DISK_H
