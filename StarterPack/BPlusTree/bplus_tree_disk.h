#ifndef BPLUS_TREE_DISK_H
#define BPLUS_TREE_DISK_H

#include <iostream>

#define PAGE_SIZE 4096
#define PAYLOAD_SIZE 60 // Kích thước chuỗi dữ liệu giả lập, bao gồm cả ký tự kết thúc chuỗi '\0'

/* =========================================================
   TÍNH TOÁN DUNG LƯỢNG NODE ĐỂ KHÔNG VƯỢT QUÁ 4096 BYTES
   ========================================================= */
// Kích thước cố định chung: 
// is_leaf (1 byte) + bool padding (3 bytes) + num_keys (4 bytes) = 8 bytes.
// Khoảng trống còn lại cho Union: 4096 - 8 = 4088 bytes.

// 1. ĐỐI VỚI NODE LÁ (LEAF NODES)
// Mỗi Record chứa: ID (4 bytes) + Payload (60 bytes) = 64 bytes.
// Node Lá cũng cần 1 con trỏ next_leaf_offset (4 bytes).
// Phương trình: 8 + 64 * MAX_LEAF_KEYS + 4 <= 4096  => MAX_LEAF_KEYS <= 63.8
// Chọn MAX_LEAF_KEYS = 60 cho an toàn, để lại một khoảng đệm nhỏ.
#define MAX_LEAF_KEYS 60

// 2. ĐỐI VỚI NODE TRONG (INTERNAL NODES)
// Chỉ chứa Keys (4 bytes) và Child_Offsets (4 bytes).
// Phương trình: 8 + 4 * MAX_INTERNAL_KEYS + 4 * (MAX_INTERNAL_KEYS + 1) <= 4096 
// => 8 * MAX_INTERNAL_KEYS + 12 <= 4096 
// => MAX_INTERNAL_KEYS <= 510.5
// CẢNH BÁO: MAX_INTERNAL_KEYS KHÔNG ĐƯỢC vượt quá 510. Nếu đặt là 511, kích thước sizeof(BPlusNode) 
// sẽ trở thành 4100 bytes (> 4096), gây ra lỗi ghi đè dữ liệu rác trên đĩa cứng một cách thầm lặng.
#define MAX_INTERNAL_KEYS 510

/* ========================================================= */

// Cấu trúc cho một dòng dữ liệu (Record) được lưu trữ trong Node Lá
struct Record {
    int id;
    char payload[PAYLOAD_SIZE];
};

// Cấu trúc Node chuẩn của B+ Tree trên ổ đĩa
struct BPlusNode {
    bool is_leaf;       // true nếu là Node Lá, false nếu là Node Trong
    char padding1[3];   // Padding bytes để căn chỉnh bộ nhớ (ngăn ngừa lỗi khi biên dịch chéo qua nhiều OS/Trình biên dịch khác nhau)
    int num_keys;       // Số lượng keys hiện tại đang chứa

    // Sử dụng UNION: Các cấu trúc con sẽ chia sẻ chung một không gian bộ nhớ vật lý.
    // Kích thước của union sẽ bằng kích thước của cấu trúc con lớn nhất.
    // Nếu is_leaf = true, mảng records sẽ được dùng. Nếu is_leaf = false, mảng internal sẽ được dùng.
    union {
        // Cấu trúc cho Node Trong (KHÔNG chứa payload)
        struct {
            int keys[MAX_INTERNAL_KEYS];
            int children_offsets[MAX_INTERNAL_KEYS + 1];
        } internal;

        // Cấu trúc cho Node Lá (CÓ CHỨA Payload và con trỏ next_leaf)
        struct {
            Record records[MAX_LEAF_KEYS];
            int next_leaf_offset; // Vị trí Offset trỏ tới Node Lá tiếp theo (hỗ trợ cho Range Query)
        } leaf;
    };
};

// Cấu trúc Header nằm ở ngay đầu file index.dat (vị trí offset = 0)
// Cấu trúc này được độn thêm (padding) cho tròn đúng 1 PAGE_SIZE (4096 bytes) để đảm bảo các node phía sau
// khớp hoàn hảo với các sector trên ổ đĩa vật lý.
struct DBHeader {
    int root_offset;       // Vị trí Byte offset của Root Node hiện tại
    int total_nodes;       // Tổng số node đã được tạo
    int free_list_offset;  // Vị trí Disk offset của trang trống có thể tái sử dụng (-1 nếu không có)
    
    // Padding để lấp đầy chính xác 4KB (4096 bytes - 3 biến int * 4 bytes = 4084 bytes)
    char reserved[PAGE_SIZE - 12]; 
};

#endif // BPLUS_TREE_DISK_H
