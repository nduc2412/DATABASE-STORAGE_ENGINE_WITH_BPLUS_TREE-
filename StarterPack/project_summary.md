# Tóm Tắt Cấu Trúc Dự Án: B+ Tree và True B-Tree Benchmark

Tài liệu này tóm tắt toàn bộ cấu trúc mã nguồn, vai trò của từng file và phân tích ngắn gọn các thuật toán cốt lõi được sử dụng để đánh giá hiệu năng (Benchmark) giữa B+ Tree và True B-Tree.

---

## 1. Cấu Trúc File & Nội Dung

Dự án được chia thành các module độc lập để dễ quản lý:

- **`main.cpp`**: Thành phần cốt lõi quản lý quy trình đo lường (Benchmark Engine).
  - Nhiệm vụ: Đọc file `dataset_large.csv`, nạp dữ liệu (import) vào hai cấu trúc cây B+ Tree và B-Tree được lưu trên ổ đĩa. Thiết lập đo lường thời gian (nanoseconds) và số lượt Disk Reads để so sánh hiệu suất.
  
- **`utils.h` & `utils.cpp`**: 
  - Nhiệm vụ: Chứa các biến toàn cục (con trỏ file, biến đếm I/O) và các hàm tìm kiếm cơ sở (Linear Search, Binary Search) được tái sử dụng trên toàn bộ dự án.

- **`bplus_tree_disk.h`**: 
  - Nhiệm vụ: Định nghĩa cấu trúc vật lý của B+ Tree trên ổ đĩa. Sử dụng cấu trúc `union` để tối ưu kích thước `BPlusNode` đạt chuẩn 4KB Page Size. Node trong (Internal) chỉ chứa Key (Fanout đạt 510), Node lá (Leaf) chứa Record và con trỏ `next_leaf`.

- **`bplus_tree.h` & `bplus_tree.cpp`**:
  - Nhiệm vụ: Cài đặt các thao tác I/O (`readNode`, `writeNode`) và các thuật toán đặc thù của B+ Tree (Insert, Point Query, Range Query).

- **`btree_disk.h`**:
  - Nhiệm vụ: Định nghĩa cấu trúc vật lý của True B-Tree. Do phải lưu trữ toàn bộ dữ liệu (Payload) tại các node trong, số lượng Record chứa được trên mỗi node giảm xuống (Fanout = 60).

- **`btree.h` & `btree.cpp`**:
  - Nhiệm vụ: Cài đặt các thao tác I/O (`readBNode`, `writeBNode`) và các thuật toán cốt lõi của True B-Tree (Point Query với khả năng Early Exit, Range Query thông qua In-Order Traversal, và thuật toán Split).

- **`data_generator.cpp`**:
  - Nhiệm vụ: Tập lệnh sinh dữ liệu giả lập ngẫu nhiên và xuất ra file `dataset_large.csv`.

---

## 2. Nhiệm Vụ Của Từng Hàm & Thuật Toán Chính

### A. Point Query (Truy vấn điểm)
- **`pointQueryBPlusStyle(target_id)`**:
  - *Thuật toán*: Bắt đầu từ Root, duyệt theo chiều sâu dựa vào việc so sánh Key. Do đặc trưng phân nhánh rộng (Fanout lớn), cây có chiều cao thấp giúp thao tác duyệt diễn ra nhanh chóng. Phép so khớp kết quả cuối cùng luôn được thực hiện tại Node Lá (Leaf). 
  - *Đặc điểm*: Số lượt Disk Read luôn tỉ lệ thuận với chiều cao của cây (O(log_511(N))).

- **`pointQueryBTreeStyle(target_id)`**:
  - *Thuật toán*: Bắt đầu duyệt từ Root. Khác với B+ Tree, nếu tìm thấy ID trùng khớp ngay tại Node Trong (Internal Node), thuật toán sẽ ghi nhận kết quả và kết thúc sớm (Early Exit) mà không cần duyệt tiếp xuống tầng dưới.
  - *Đặc điểm*: Dù có cơ chế Early Exit, do hệ số phân nhánh thấp khiến cây có chiều cao lớn (O(log_60(N))), số lượt Disk Read trung bình vẫn cao hơn đáng kể so với B+ Tree.

### B. Range Query (Truy vấn theo khoảng)
- **`rangeQueryBPlusStyle(start_id, end_id)`**:
  - *Thuật toán*: 
    1. Áp dụng thuật toán Point Query để tìm vị trí Node Lá đầu tiên chứa `start_id`.
    2. Sử dụng con trỏ `next_leaf_offset` lặp tuần tự qua các Node Lá liền kề cho đến khi gặp giá trị lớn hơn `end_id`.
  - *Đặc điểm*: Tận dụng hiệu quả cơ chế truy xuất tuần tự (Sequential I/O). Việc đọc lượng lớn Record tốn ít Disk Read do ổ đĩa chỉ việc lướt qua các khối dữ liệu (blocks) nằm kế tiếp nhau.

- **`rangeQueryBTreeRecursive(start_id, end_id)`**:
  - *Thuật toán*: Áp dụng thuật toán In-Order Traversal (Duyệt trung thứ tự). Đệ quy theo quy tắc: 
    1. Đi xuống nhánh con bên Trái.
    2. Xử lý Record hiện tại.
    3. Đi xuống nhánh con bên Phải.
  - *Đặc điểm*: Gây ra tình trạng truy xuất ngẫu nhiên (Random I/O) với tần suất lớn. Khi quét một khoảng dữ liệu rộng, thuật toán liên tục phải quay lui (backtrack) và truy cập đĩa ở nhiều vị trí không liền kề, làm tăng đáng kể tổng số lượt Disk Read.

### C. Insertion (Thuật toán Thêm mới)
- **`insertRecursive` (B+ Tree Split)**:
  - Khi một Node Lá đầy (đạt 63 records), node này được tách làm đôi. Bản sao của Key ở giữa được đẩy lên Node cha để phục vụ mục đích định tuyến.
- **`insertBTreeRecursive` (True B-Tree Split)**:
  - Khi một Node đầy (đạt 60 records), nó được tách làm đôi. Toàn bộ Record ở vị trí trung vị (bao gồm cả Key và Payload) được thăng cấp và chuyển lên Node cha. Cơ chế này làm tăng lượng dữ liệu cần lưu trữ ở các Node Trong, dẫn đến việc cây tăng chiều cao nhanh hơn.

### D. Các Hàm Hỗ Trợ
- **`allocateNode()` / `allocateBNode()`**: Cấp phát không gian lưu trữ mới bằng cách ghi nối tiếp vào cuối file nhị phân (append), sau đó cập nhật trực tiếp DBHeader.
- **`linearSearch` / `binarySearch`**: Áp dụng tìm kiếm trên bộ nhớ trong (In-memory search) tại từng Node. Do dữ liệu đã được nạp vào RAM, Binary Search giúp tối ưu thời gian xử lý của CPU nhưng không làm thay đổi số lượt thao tác với ổ đĩa (Disk Read).
