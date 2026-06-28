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

bool insertBTreeRecursive(int current_offset, int id, const char* payload, Record& promoted_record, int& new_offset) {
    BNode node;
    readBNode(current_offset, node);

    int pos = 0;
    while (pos < node.num_keys && node.records[pos].id < id) pos++;
    
    if (pos < node.num_keys && node.records[pos].id == id) return false;

    if (node.is_leaf) {
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
            BNode new_node;
            memset(&new_node, 0, sizeof(BNode));
            new_node.is_leaf = true;
            
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

            int mid = MAX_BTREE_KEYS / 2;
            
            node.num_keys = mid;
            promoted_record = temp_records[mid];
            
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
    } else {
        Record child_promoted_record;
        int child_new_offset;
        bool split = insertBTreeRecursive(node.children_offsets[pos], id, payload, child_promoted_record, child_new_offset);

        if (!split) return false;

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
            BNode new_node;
            memset(&new_node, 0, sizeof(BNode));
            new_node.is_leaf = false;

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

void insertBTreeRecord(int id, const char* payload) {
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

bool pointQueryBTreeStyle(int target_id, bool use_binary_search) {
    if (btree_header.root_offset == -1) return false;
    
    int current_offset = btree_header.root_offset;
    BNode node;
    
    while (current_offset != -1) {
        readBNode(current_offset, node);
        int idx = use_binary_search ? binarySearch(node.records, node.num_keys, target_id) 
                                    : linearSearch(node.records, node.num_keys, target_id);
        
        if (idx < node.num_keys && node.records[idx].id == target_id) {
            return true; 
        }
        
        if (node.is_leaf) {
            return false;
        } else {
            current_offset = node.children_offsets[idx];
        }
    }
    return false;
}

int rangeQueryBTreeRecursive(int offset, int start_id, int end_id, bool use_binary_search) {
    if (offset == -1) return 0;
    BNode node;
    readBNode(offset, node);
    
    int count = 0;
    int start_idx = use_binary_search ? binarySearch(node.records, node.num_keys, start_id)
                                      : linearSearch(node.records, node.num_keys, start_id);
    int end_idx_limit = use_binary_search ? binarySearch(node.records, node.num_keys, end_id + 1)
                                          : linearSearch(node.records, node.num_keys, end_id + 1);

    if (node.is_leaf) {
        for (int i = start_idx; i < node.num_keys; ++i) {
            if (node.records[i].id <= end_id) {
                if (node.records[i].id >= start_id) count++;
            } else {
                break;
            }
        }
    } else {
        for (int i = start_idx; i <= end_idx_limit; ++i) {
            count += rangeQueryBTreeRecursive(node.children_offsets[i], start_id, end_id, use_binary_search);
            if (i < end_idx_limit && i < node.num_keys) {
                if (node.records[i].id >= start_id && node.records[i].id <= end_id) {
                    count++;
                }
            }
        }
    }
    return count;
}

int rangeQueryBTreeStyle(int start_id, int end_id, bool use_binary_search) {
    if (btree_header.root_offset == -1) return 0;
    return rangeQueryBTreeRecursive(btree_header.root_offset, start_id, end_id, use_binary_search);
}
