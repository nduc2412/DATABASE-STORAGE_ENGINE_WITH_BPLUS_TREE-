#ifndef BTREE_DISK_H
#define BTREE_DISK_H

#include "../BPlusTree/bplus_tree_disk.h"

#define MAX_BTREE_KEYS 60

// Cấu trúc Node chuẩn của True B-Tree trên ổ đĩa
struct BNode {
    bool is_leaf;       // true nếu là Node Lá, false nếu là Node Trong
    char padding1[3];   // Padding bytes để căn chỉnh bộ nhớ
    int num_keys;       // Số lượng record hiện tại đang chứa

    // Trong True B-Tree, CẢ Node Trong và Node Lá đều chứa toàn bộ Record (Key + Payload)
    Record records[MAX_BTREE_KEYS];
    
    // Mảng các vị trí Offset trỏ tới các Node con để định tuyến
    int children_offsets[MAX_BTREE_KEYS + 1];

    // Padding để lấp đầy chính xác 4KB (4096 bytes)
    // Tính toán: 8 + 60*64 + 61*4 = 4092 bytes. Cần 4 bytes padding.
    char reserved[4];
};

#endif // BTREE_DISK_H
