#include "bplus_tree.h"
#include "../utils.h"
#include <cstring>

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

// Đệ quy chèn Record mới vào B+ Tree.
// Trả về true nếu Node hiện tại bị Split (tách đôi), đồng thời truyền new_key và new_offset lên cho Node cha.
bool insertRecursive(int current_offset, int id, const char* payload, int& new_key, int& new_offset) {
    BPlusNode node;
    readNode(current_offset, node);

    // TRƯỜNG HỢP 1: XỬ LÝ TẠI NODE LÁ (LEAF NODE)
    if (node.is_leaf) {
        // Tìm vị trí chèn thích hợp
        int pos = 0;
        while (pos < node.num_keys && node.leaf.records[pos].id < id) pos++;
        
        // Nếu ID đã tồn tại thì bỏ qua (không chèn trùng)
        if (pos < node.num_keys && node.leaf.records[pos].id == id) return false;

        // Nếu Node chưa đầy, chèn trực tiếp Record vào vị trí tìm được
        if (node.num_keys < MAX_LEAF_KEYS) {
            for (int i = node.num_keys; i > pos; --i) {
                node.leaf.records[i] = node.leaf.records[i - 1];
            }
            node.leaf.records[pos].id = id;
            strncpy(node.leaf.records[pos].payload, payload, PAYLOAD_SIZE - 1);
            node.leaf.records[pos].payload[PAYLOAD_SIZE - 1] = '\0';
            node.num_keys++;
            writeNode(current_offset, node);
            return false;
        } else {
            // Nếu Node đã đầy, tiến hành Split (Tách Node Lá)
            BPlusNode new_node;
            memset(&new_node, 0, sizeof(BPlusNode));
            new_node.is_leaf = true;
            
            // Đưa tất cả records (bao gồm cả record mới) vào mảng tạm
            Record temp_records[MAX_LEAF_KEYS + 1];
            for (int i = 0, j = 0; i < MAX_LEAF_KEYS + 1; ++i) {
                if (i == pos) {
                    temp_records[i].id = id;
                    strncpy(temp_records[i].payload, payload, PAYLOAD_SIZE - 1);
                    temp_records[i].payload[PAYLOAD_SIZE - 1] = '\0';
                } else {
                    temp_records[i] = node.leaf.records[j++];
                }
            }

            // Chia đôi mảng tạm ra 2 node
            int mid = (MAX_LEAF_KEYS + 1) / 2;
            node.num_keys = mid;
            new_node.num_keys = (MAX_LEAF_KEYS + 1) - mid;

            for (int i = 0; i < node.num_keys; ++i) {
                node.leaf.records[i] = temp_records[i];
            }
            for (int i = 0; i < new_node.num_keys; ++i) {
                new_node.leaf.records[i] = temp_records[mid + i];
            }

            // Cập nhật danh sách liên kết (Linked List) giữa các lá
            int right_offset = allocateNode();
            new_node.leaf.next_leaf_offset = node.leaf.next_leaf_offset;
            node.leaf.next_leaf_offset = right_offset;

            // Lấy Key nhỏ nhất của Node bên phải để thăng cấp (Promote) lên cha
            new_key = new_node.leaf.records[0].id;
            new_offset = right_offset;

            writeNode(current_offset, node);
            writeNode(right_offset, new_node);
            return true;
        }
    } 
    // TRƯỜNG HỢP 2: XỬ LÝ TẠI NODE TRONG (INTERNAL NODE)
    else {
        // Định tuyến tìm Node con phù hợp để đi xuống
        int pos = 0;
        while (pos < node.num_keys && node.internal.keys[pos] <= id) pos++;

        // Gọi đệ quy xuống Node con
        int child_offset = node.internal.children_offsets[pos];
        int child_new_key, child_new_offset;
        bool split = insertRecursive(child_offset, id, payload, child_new_key, child_new_offset);

        // Nếu Node con không tách, không cần xử lý gì thêm
        if (!split) return false;

        // Nếu Node con tách và đẩy 1 Key lên, chèn Key đó vào Node hiện tại
        if (node.num_keys < MAX_INTERNAL_KEYS) {
            for (int i = node.num_keys; i > pos; --i) {
                node.internal.keys[i] = node.internal.keys[i - 1];
                node.internal.children_offsets[i + 1] = node.internal.children_offsets[i];
            }
            node.internal.keys[pos] = child_new_key;
            node.internal.children_offsets[pos + 1] = child_new_offset;
            node.num_keys++;
            writeNode(current_offset, node);
            return false;
        } else {
            // Nếu Node hiện tại đầy, tiến hành Split (Tách Node Trong)
            BPlusNode new_node;
            memset(&new_node, 0, sizeof(BPlusNode));
            new_node.is_leaf = false;

            // Chép toàn bộ keys và offsets (cùng với key mới) vào mảng tạm
            int temp_keys[MAX_INTERNAL_KEYS + 1];
            int temp_children[MAX_INTERNAL_KEYS + 2];

            for (int i = 0, j = 0; i < MAX_INTERNAL_KEYS + 1; ++i) {
                if (i == pos) {
                    temp_keys[i] = child_new_key;
                } else {
                    temp_keys[i] = node.internal.keys[j++];
                }
            }
            for (int i = 0, j = 0; i < MAX_INTERNAL_KEYS + 2; ++i) {
                if (i == pos + 1) {
                    temp_children[i] = child_new_offset;
                } else {
                    temp_children[i] = node.internal.children_offsets[j++];
                }
            }

            // Chia đôi, lấy Key ở chính giữa (median) để tiếp tục thăng cấp
            int mid = (MAX_INTERNAL_KEYS + 1) / 2;
            node.num_keys = mid;
            new_node.num_keys = MAX_INTERNAL_KEYS + 1 - mid - 1;

            new_key = temp_keys[mid];

            // Chép dữ liệu về lại 2 node
            for (int i = 0; i < node.num_keys; ++i) {
                node.internal.keys[i] = temp_keys[i];
                node.internal.children_offsets[i] = temp_children[i];
            }
            node.internal.children_offsets[node.num_keys] = temp_children[node.num_keys];

            for (int i = 0; i < new_node.num_keys; ++i) {
                new_node.internal.keys[i] = temp_keys[mid + 1 + i];
                new_node.internal.children_offsets[i] = temp_children[mid + 1 + i];
            }
            new_node.internal.children_offsets[new_node.num_keys] = temp_children[MAX_INTERNAL_KEYS + 1];

            int right_offset = allocateNode();
            new_offset = right_offset;

            writeNode(current_offset, node);
            writeNode(right_offset, new_node);
            return true;
        }
    }
}

// Hàm khởi tạo và gọi đệ quy chèn Record vào B+ Tree
void insertRecord(int id, const char* payload) {
    // Nếu cây trống, tạo Root Node đầu tiên (là Node Lá)
    if (header.root_offset == -1) {
        int root_offset = allocateNode();
        BPlusNode root;
        memset(&root, 0, sizeof(BPlusNode));
        root.is_leaf = true;
        root.num_keys = 1;
        root.leaf.records[0].id = id;
        strncpy(root.leaf.records[0].payload, payload, PAYLOAD_SIZE - 1);
        root.leaf.records[0].payload[PAYLOAD_SIZE - 1] = '\0';
        root.leaf.next_leaf_offset = -1;
        writeNode(root_offset, root);
        
        header.root_offset = root_offset;
        fseek(db_file, 0, SEEK_SET);
        fwrite(&header, sizeof(DBHeader), 1, db_file);
        return;
    }

    // Gọi hàm chèn đệ quy từ Root
    int new_key, new_offset;
    if (insertRecursive(header.root_offset, id, payload, new_key, new_offset)) {
        // Nếu Root bị tách, tạo Root mới (Node Trong) trỏ tới 2 Node con
        int new_root_offset = allocateNode();
        BPlusNode new_root;
        memset(&new_root, 0, sizeof(BPlusNode));
        new_root.is_leaf = false;
        new_root.num_keys = 1;
        new_root.internal.keys[0] = new_key;
        new_root.internal.children_offsets[0] = header.root_offset;
        new_root.internal.children_offsets[1] = new_offset;
        writeNode(new_root_offset, new_root);
        
        header.root_offset = new_root_offset;
        fseek(db_file, 0, SEEK_SET);
        fwrite(&header, sizeof(DBHeader), 1, db_file);
    }
}

// B+ Tree Point Query (Truy vấn 1 điểm)
bool pointQueryBPlusStyle(int target_id, bool use_binary_search) {
    if (header.root_offset == -1) return false;
    int offset = header.root_offset; 
    BPlusNode node; 
    
    // Trượt liên tục từ Root xuống tới tận Node Lá
    while (true) {
        readNode(offset, node); 
        if (node.is_leaf) {
            // Tại Node Lá, tìm kiếm đích danh ID
            int idx = use_binary_search ? binarySearch(node.leaf.records, node.num_keys, target_id) : linearSearch(node.leaf.records, node.num_keys, target_id); 
            if (idx < node.num_keys && node.leaf.records[idx].id == target_id) return true; 
            return false; 
        }
        else {
            // Tại Node Trong, tìm chỉ mục con đường để đi xuống
            int idx = use_binary_search ? binarySearch(node.internal.keys, node.num_keys, target_id) : linearSearch(node.internal.keys, node.num_keys, target_id); 
            if (idx < node.num_keys && node.internal.keys[idx]  == target_id) idx++; 
            offset = node.internal.children_offsets[idx]; 
        }
    }
}

// B+ Tree Range Query (Truy vấn dải)
int rangeQueryBPlusStyle(int start_id, int end_id, bool use_binary_search) {
    if (header.root_offset == -1) return 0;
    
    int current_offset = header.root_offset;
    BPlusNode node;
    
    // Duyệt thẳng xuống Node Lá đầu tiên chứa vùng start_id
    while (true) {
        readNode(current_offset, node);
        if (node.is_leaf) break;
        
        int idx = use_binary_search ? binarySearch(node.internal.keys, node.num_keys, start_id + 1)
                                    : linearSearch(node.internal.keys, node.num_keys, start_id + 1);
        current_offset = node.internal.children_offsets[idx];
    }
    
    int count = 0;
    // Tận dụng danh sách liên kết, trượt ngang qua các Lá
    while (current_offset != -1) {
        int start_idx = use_binary_search ? binarySearch(node.leaf.records, node.num_keys, start_id)
                                          : linearSearch(node.leaf.records, node.num_keys, start_id);
        
        bool reached_end = false;
        // Quét các records trong Lá hiện tại
        for (int i = start_idx; i < node.num_keys; ++i) {
            if (node.leaf.records[i].id >= start_id && node.leaf.records[i].id <= end_id) {
                count++;
            } else if (node.leaf.records[i].id > end_id) {
                reached_end = true;
                break;
            }
        }
        
        // Nếu đã quét qua điểm kết thúc, dừng lại hoàn toàn
        if (reached_end) break;
        
        // Đọc Node Lá tiếp theo thông qua con trỏ
        current_offset = node.leaf.next_leaf_offset;
        if (current_offset != -1) {
            readNode(current_offset, node);
        }
    }
    return count;
}
