#include "btree.h"
#include "../utils.h"
#include <cstring>

void readBNode(int offset, BNode& node) {
    if (!btree_db_file) return; 
    fseek(btree_db_file, offset, SEEK_SET); 
    fread(&node, sizeof(BNode), 1, btree_db_file); 
    btree_read_count++; 
}

void writeBNode(int offset, BNode& node) {
    if (!btree_db_file) return; 
    fseek(btree_db_file, offset, SEEK_SET); 
    fwrite(&node, sizeof(BNode), 1, btree_db_file); 
    btree_write_count++; 
}

int allocateBNode() {
    int offset = sizeof(DBHeader) + btree_header.total_nodes * sizeof(BNode); 
    btree_header.total_nodes++; 
    fseek(btree_db_file, 0, SEEK_SET); 
    fwrite(&btree_header, sizeof(DBHeader), 1, btree_db_file);
    BNode node; 
    memset(&node, 0, sizeof(BNode)); 
    fseek(btree_db_file, offset, SEEK_SET); 
    fwrite(&node, sizeof(BNode), 1, btree_db_file); 
    return offset; 
}

// Đệ quy chèn Record mới vào True B-Tree
// Trả về true nếu Node hiện tại bị Split (tách đôi), đồng thời truyền promoted_record và new_offset lên cho Node cha
bool insertBTreeRecursive(int current_offset, int id, const char* payload, Record& promoted_record, int& new_offset) {
    BNode node;
    readBNode(current_offset, node);

    // Tìm vị trí chèn thích hợp
    int pos = 0;
    while (pos < node.num_keys && node.records[pos].id < id) pos++;
    
    // Bỏ qua nếu ID đã tồn tại trong Node
    if (pos < node.num_keys && node.records[pos].id == id) return false;

    // TRƯỜNG HỢP 1: XỬ LÝ TẠI NODE LÁ
    if (node.is_leaf) {
        // Nếu Node Lá chưa đầy, chèn trực tiếp Record vào mảng
        if (node.num_keys < MAX_BTREE_KEYS) {
            for (int i = node.num_keys; i > pos; --i) {
                node.records[i] = node.records[i - 1];
            }
            node.records[pos].id = id;
            strncpy(node.records[pos].payload, payload, PAYLOAD_SIZE - 1);
            node.records[pos].payload[PAYLOAD_SIZE - 1] = '\0';
            node.num_keys++;
            writeBNode(current_offset, node);
            return false;
        } else {
            // Tách Node Lá khi mảng đã đầy
            BNode new_node;
            memset(&new_node, 0, sizeof(BNode));
            new_node.is_leaf = true;
            
            // Dồn tất cả Record vào mảng tạm để dễ phân chia
            Record temp_records[MAX_BTREE_KEYS + 1];
            for (int i = 0, j = 0; i < MAX_BTREE_KEYS + 1; ++i) {
                if (i == pos) {
                    temp_records[i].id = id;
                    strncpy(temp_records[i].payload, payload, PAYLOAD_SIZE - 1);
                    temp_records[i].payload[PAYLOAD_SIZE - 1] = '\0';
                } else {
                    temp_records[i] = node.records[j++];
                }
            }

            // Chia đôi Node và rút phần tử ở giữa thăng cấp lên làm rào chắn
            int mid = MAX_BTREE_KEYS / 2;
            
            node.num_keys = mid;
            promoted_record = temp_records[mid]; // Khác B+ Tree, B-Tree nhấc bổng toàn bộ Record lên
            
            new_node.num_keys = MAX_BTREE_KEYS - mid;
            for (int i = 0; i < node.num_keys; ++i) {
                node.records[i] = temp_records[i];
            }
            for (int i = 0; i < new_node.num_keys; ++i) {
                new_node.records[i] = temp_records[mid + 1 + i];
            }

            int right_offset = allocateBNode();
            new_offset = right_offset;
            writeBNode(current_offset, node);
            writeBNode(right_offset, new_node);
            return true;
        }
    } 
    // TRƯỜNG HỢP 2: XỬ LÝ TẠI NODE TRONG
    else {
        Record child_promoted_record;
        int child_new_offset;
        
        // Gọi đệ quy xuống Node con tương ứng
        bool split = insertBTreeRecursive(node.children_offsets[pos], id, payload, child_promoted_record, child_new_offset);

        // Không làm gì nếu Node con không bị tách
        if (!split) return false;

        // Nếu Node con tách và thăng cấp một Record lên, tiến hành chèn vào Node hiện tại
        if (node.num_keys < MAX_BTREE_KEYS) {
            for (int i = node.num_keys; i > pos; --i) {
                node.records[i] = node.records[i - 1];
                node.children_offsets[i + 1] = node.children_offsets[i];
            }
            node.records[pos] = child_promoted_record;
            node.children_offsets[pos + 1] = child_new_offset;
            node.num_keys++;
            writeBNode(current_offset, node);
            return false;
        } else {
            // Tách Node Trong khi mảng đã đầy
            BNode new_node;
            memset(&new_node, 0, sizeof(BNode));
            new_node.is_leaf = false;

            // Dồn tất cả Record và đường dẫn Offset vào mảng tạm
            Record temp_records[MAX_BTREE_KEYS + 1];
            int temp_children[MAX_BTREE_KEYS + 2];

            for (int i = 0, j = 0; i < MAX_BTREE_KEYS + 1; ++i) {
                if (i == pos) {
                    temp_records[i] = child_promoted_record;
                } else {
                    temp_records[i] = node.records[j++];
                }
            }
            for (int i = 0, j = 0; i < MAX_BTREE_KEYS + 2; ++i) {
                if (i == pos + 1) {
                    temp_children[i] = child_new_offset;
                } else {
                    temp_children[i] = node.children_offsets[j++];
                }
            }

            // Chia đôi Node Trong và rút phần tử ở giữa thăng cấp tiếp
            int mid = MAX_BTREE_KEYS / 2;
            node.num_keys = mid;
            promoted_record = temp_records[mid];
            new_node.num_keys = MAX_BTREE_KEYS - mid;

            for (int i = 0; i < node.num_keys; ++i) {
                node.records[i] = temp_records[i];
                node.children_offsets[i] = temp_children[i];
            }
            node.children_offsets[node.num_keys] = temp_children[node.num_keys];

            for (int i = 0; i < new_node.num_keys; ++i) {
                new_node.records[i] = temp_records[mid + 1 + i];
                new_node.children_offsets[i] = temp_children[mid + 1 + i];
            }
            new_node.children_offsets[new_node.num_keys] = temp_children[MAX_BTREE_KEYS + 1];

            int right_offset = allocateBNode();
            new_offset = right_offset;

            writeBNode(current_offset, node);
            writeBNode(right_offset, new_node);
            return true;
        }
    }
}

// Wrapper tạo Root cho True B-Tree
void insertBTreeRecord(int id, const char* payload) {
    // Khởi tạo Root đầu tiên dưới dạng Node Lá
    if (btree_header.root_offset == -1) {
        int root_offset = allocateBNode();
        BNode root;
        memset(&root, 0, sizeof(BNode));
        root.is_leaf = true;
        root.num_keys = 1;
        root.records[0].id = id;
        strncpy(root.records[0].payload, payload, PAYLOAD_SIZE - 1);
        root.records[0].payload[PAYLOAD_SIZE - 1] = '\0';
        writeBNode(root_offset, root);
        
        btree_header.root_offset = root_offset;
        fseek(btree_db_file, 0, SEEK_SET);
        fwrite(&btree_header, sizeof(DBHeader), 1, btree_db_file);
        return;
    }

    // Nếu Root cũ bị quá tải và tách làm đôi, tạo Root mới (Node Trong) chứa bản thăng cấp
    Record promoted_record;
    int new_offset;
    if (insertBTreeRecursive(btree_header.root_offset, id, payload, promoted_record, new_offset)) {
        int new_root_offset = allocateBNode();
        BNode new_root;
        memset(&new_root, 0, sizeof(BNode));
        new_root.is_leaf = false;
        new_root.num_keys = 1;
        new_root.records[0] = promoted_record;
        new_root.children_offsets[0] = btree_header.root_offset;
        new_root.children_offsets[1] = new_offset;
        writeBNode(new_root_offset, new_root);
        
        btree_header.root_offset = new_root_offset;
        fseek(btree_db_file, 0, SEEK_SET);
        fwrite(&btree_header, sizeof(DBHeader), 1, btree_db_file);
    }
}

// True B-Tree Point Query (Truy vấn 1 điểm với cơ chế Early Exit)
bool pointQueryBTreeStyle(int target_id, bool use_binary_search) {
    if (btree_header.root_offset == -1) return false;
    
    int current_offset = btree_header.root_offset;
    BNode node;
    
    // Duyệt qua từng tầng của cây để tìm ID
    while (current_offset != -1) {
        readBNode(current_offset, node);
        int idx = use_binary_search ? binarySearch(node.records, node.num_keys, target_id) 
                                    : linearSearch(node.records, node.num_keys, target_id);
        
        // EARLY EXIT: Điểm ăn tiền của True B-Tree. Ngay khi tìm thấy ở Node Trong thì chốt kết quả và thoát ngay.
        if (idx < node.num_keys && node.records[idx].id == target_id) {
            return true; 
        }
        
        // Dừng lại nếu đã chạm tới Node Lá mà vẫn không tìm thấy
        if (node.is_leaf) {
            return false;
        } else {
            // Định tuyến đi xuống Node con tương ứng
            current_offset = node.children_offsets[idx];
        }
    }
    return false;
}

// Hàm đệ quy duyệt In-Order Traversal hỗ trợ truy vấn dải
int rangeQueryBTreeRecursive(int offset, int start_id, int end_id, bool use_binary_search) {
    if (offset == -1) return 0;
    BNode node;
    readBNode(offset, node);
    
    int count = 0;
    
    // Tìm chặn dưới và chặn trên cho phạm vi tìm kiếm trong Node hiện tại
    int start_idx = use_binary_search ? binarySearch(node.records, node.num_keys, start_id)
                                      : linearSearch(node.records, node.num_keys, start_id);
    int end_idx_limit = use_binary_search ? binarySearch(node.records, node.num_keys, end_id + 1)
                                          : linearSearch(node.records, node.num_keys, end_id + 1);

    // Thu thập kết quả từ Node Lá
    if (node.is_leaf) {
        for (int i = start_idx; i < node.num_keys; ++i) {
            if (node.records[i].id <= end_id) {
                if (node.records[i].id >= start_id) count++;
            } else {
                break;
            }
        }
    } 
    // Tiến hành duyệt In-Order (Trái - Giữa - Phải) xuyên suốt cây
    else {
        for (int i = start_idx; i <= end_idx_limit; ++i) {
            count += rangeQueryBTreeRecursive(node.children_offsets[i], start_id, end_id, use_binary_search);
            
            // Xử lý Record trung gian bị kẹp giữa 2 Node con
            if (i < end_idx_limit && i < node.num_keys) {
                if (node.records[i].id >= start_id && node.records[i].id <= end_id) {
                    count++;
                }
            }
        }
    }
    return count;
}

// B-Tree Range Query (Truy vấn dải bằng phương pháp In-Order Traversal)
int rangeQueryBTreeStyle(int start_id, int end_id, bool use_binary_search) {
    if (btree_header.root_offset == -1) return 0;
    return rangeQueryBTreeRecursive(btree_header.root_offset, start_id, end_id, use_binary_search);
}
