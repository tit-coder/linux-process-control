# Test Cases Documentation - Linux Process Control

Dự án: **Process Management System – Suspend / Resume Process using Signal**  
Môn học: **Lập trình hệ thống (System Programming)**  
Nhóm: **Nhóm 4 sinh viên (Leader: tit-coder)**  
Hệ điều hành kiểm thử: **Ubuntu 24.04.4 LTS (Linux kernel 6.17 x86_64)**  
Bộ biên dịch: **GCC 13.3.0 (C11, -std=c11 -D_POSIX_C_SOURCE=200809L)**  

---

## 1. Danh sách tổng quan các kịch bản kiểm thử

| Test ID | Module | Mục tiêu kiểm thử | Script tự động | Kết quả thực tế |
| :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Process Manager | Tạo tiến trình con (Worker) bằng `fork()`, cấp phát và hiển thị PID | `tests/test_process.sh` | **PASS** |
| **TC-02** | Process Manager | Dọn dẹp Worker khi cha thoát, kiểm tra phòng chống Zombie / Orphan | `tests/test_process.sh` | **PASS** |
| **TC-03** | Signal Handler | Gửi `SIGSTOP` tạm dừng Worker, kiểm chứng trạng thái `T` qua `/proc` | `tests/test_signal.sh` | **PASS** |
| **TC-04** | Signal Handler | Gửi `SIGCONT` khôi phục Worker, kiểm chứng trạng thái `S`/`R` qua `/proc` | `tests/test_signal.sh` | **PASS** |
| **TC-05** | Monitor | Đọc và bóc tách thông tin từ `/proc/<PID>/status` | `tests/test_signal.sh` | **PASS** |
| **TC-06** | Process Manager | Gửi `SIGTERM` và thu hồi Worker bằng `waitpid()` không để lại Zombie | `tests/test_signal.sh` | **PASS** |
| **TC-07** | Input & Safety | Từ chối PID âm (`-999`), bảo vệ an toàn hệ điều hành | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-08** | Input & Safety | Từ chối PID không phải số (`abc_invalid`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-09** | Input & Safety | Từ chối gửi tín hiệu tới PID không do chương trình quản lý (PID 1 `init`/`systemd`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-10** | Monitor | Xử lý an toàn khi tra cứu PID không tồn tại (`9999999`) | `tests/test_invalid_pid.sh` | **PASS** |
| **TC-11** | Terminal UI | Xử lý menu khi nhập chuỗi ký tự hoặc số ngoài dải `[1-5]` | `tests/test_invalid_pid.sh` | **PASS** |

---

## 2. Chi tiết từng ca kiểm thử

### TC-01: Tạo tiến trình Worker bằng `fork()`
- **Tiền điều kiện:** Chương trình được biên dịch thành công qua `make`.
- **Các bước thực hiện:**
  1. Khởi chạy `./bin/process_control`.
  2. Quan sát dòng thông báo khởi tạo và in PID Worker.
- **Kết quả mong đợi:**
  - `fork()` thành công, PID trả về > 0.
  - Tiến trình con in dòng thông báo bắt đầu và bắt đầu phát nhịp đếm counter mỗi giây.
- **Kết quả thực tế:** **PASS**. Worker được spawn với PID cụ thể (ví dụ PID `5847`), ghi nhận heartbeat tick mỗi giây.

---

### TC-02: Thu hồi tiến trình con, chống Zombie và Orphan
- **Tiền điều kiện:** Worker đang hoạt động bình thường.
- **Các bước thực hiện:**
  1. Gửi lệnh thoát (Menu `5. Exit`).
  2. Kiểm tra trạng thái `/proc/<Worker_PID>/status` và bảng tiến trình hệ thống qua `ps`.
- **Kết quả mong đợi:**
  - Tiến trình cha gửi `SIGTERM` và gọi `waitpid()` thu hồi exit status của Worker.
  - Worker kết thúc hoàn toàn; không xuất hiện tiến trình ở trạng thái `Z` (Zombie) hoặc bị bỏ rơi (Orphan do PID 1 nhận nuôi).
- **Kết quả thực tế:** **PASS**. Tiến trình con được thu hồi sạch sẽ, file `/proc/<PID>` biến mất ngay sau khi thoát.

---

### TC-03: Tạm dừng tiến trình bằng `SIGSTOP` (Suspend)
- **Tiền điều kiện:** Worker đang hoạt động ở trạng thái `S` (Sleeping) hoặc `R` (Running).
- **Các bước thực hiện:**
  1. Chọn Menu `1. Suspend Process`.
  2. Quan sát màn hình terminal (bộ đếm heartbeat ngừng in).
  3. Kiểm tra trường `State:` trong `/proc/<Worker_PID>/status`.
- **Kết quả mong đợi:**
  - System call `kill(worker_pid, SIGSTOP)` trả về 0.
  - Trạng thái trong `/proc/<Worker_PID>/status` là `T (stopped)`.
  - Bộ đếm không tăng thêm giá trị.
- **Kết quả thực tế:** **PASS**. File status ghi nhận `State: T (stopped)`. Bộ đếm tạm dừng chính xác.

---

### TC-04: Tiếp tục tiến trình bằng `SIGCONT` (Resume)
- **Tiền điều kiện:** Worker đang bị tạm dừng (State `T`).
- **Các bước thực hiện:**
  1. Chọn Menu `2. Resume Process`.
  2. Quan sát màn hình terminal và file `/proc/<Worker_PID>/status`.
- **Kết quả mong đợi:**
  - System call `kill(worker_pid, SIGCONT)` trả về 0.
  - Trạng thái tiến trình chuyển sang `S (sleeping)` hoặc `R (running)`.
  - Bộ đếm counter tiếp tục tăng từ giá trị trước khi tạm dừng.
- **Kết quả thực tế:** **PASS**. State ghi nhận chuyển sang `S`, tiến trình tiếp tục in heartbeat.

---

### TC-05: Đọc và phân tích thông tin `/proc/<PID>/status`
- **Tiền điều kiện:** Worker đang tồn tại trong hệ thống.
- **Các bước thực hiện:**
  1. Chọn Menu `3. Show Process Status`.
- **Kết quả mong đợi:**
  - Đọc thành công `/proc/<PID>/status`.
  - Hiển thị bảng định dạng chuẩn gồm: `PID`, `Process Name`, `Parent PID (PPid)`, `Raw State`, `Interpreted State`, `Virtual Memory`, `Resident Set (RSS)`, `Thread Count`.
  - Diễn giải trạng thái chính xác: `T` là Stopped, `S` là Interruptible Sleep, `R` là Running/Runnable.
- **Kết quả thực tế:** **PASS**. Bảng trạng thái hiển thị đầy đủ, đẹp mắt và trực quan.

---

### TC-06: Kết thúc tiến trình bằng `SIGTERM` (Terminate)
- **Tiền điều kiện:** Worker đang chạy hoặc đang suspend.
- **Các bước thực hiện:**
  1. Chọn Menu `4. Terminate Process`.
  2. Kiểm tra lại trạng thái.
- **Kết quả mong đợi:**
  - Gửi `SIGTERM`, Worker nhận signal và kết thúc sạch sẽ.
  - Tiến trình cha gọi `waitpid()` để thu hồi mã thoát (exit code 0).
  - Trạng thái Worker chuyển sang `TERMINATED / NONE`.
- **Kết quả thực tế:** **PASS**. Worker terminated gracefully và được thu hồi ngay lập tức.

---

### TC-07, TC-08, TC-09, TC-10, TC-11: An toàn & Xác thực đầu vào (Safety & Validation)
- **TC-07 (PID âm):** Nhập `-999` -> Hệ thống từ chối ngay, không thực hiện `kill()`. Kết quả: **PASS**.
- **TC-08 (PID chữ):** Nhập `abc_invalid` -> Hệ thống phát hiện lỗi định dạng, từ chối an toàn. Kết quả: **PASS**.
- **TC-09 (PID 1 - init):** Thử can thiệp vào tiến trình của hệ điều hành -> Hệ thống phát hiện PID không thuộc quyền quản lý của chương trình, chặn gửi signal với cảnh báo Security Violation. Kết quả: **PASS**.
- **TC-10 (PID không tồn tại):** Tra cứu `/proc/9999999/status` -> Báo lỗi `Process does not exist` an toàn, không gây crash chương trình. Kết quả: **PASS**.
- **TC-11 (Menu invalid):** Nhập chuỗi ký tự hoặc số không thuộc `[1-5]` -> Báo lỗi nhập sai, không bị lặp vô tận (nhờ dùng `fgets` an toàn). Kết quả: **PASS**.
