# Linux Process Control System (C11 / POSIX)

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform](https://img.shields.io/badge/Platform-Ubuntu%20Linux-orange.svg)](https://ubuntu.com/)
[![Standard](https://img.shields.io/badge/Standard-POSIX.1--2008-green.svg)](https://pubs.opengroup.org/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

Hệ thống quản lý và điều khiển vòng đời tiến trình trên Linux sử dụng tín hiệu POSIX (**POSIX Signals**) để Tạm dừng (**Suspend**), Tiếp tục (**Resume**), Giám sát (**Monitor via `/proc`**) và Kết thúc an toàn (**Graceful Termination**) – Đồ án môn học **Lập trình hệ thống (System Programming)**.

- **GitHub Repository**: [https://github.com/tit-coder/linux-process-control](https://github.com/tit-coder/linux-process-control)
- **Nhóm sinh viên thực hiện**: Nhóm 4 thành viên
- **Trưởng nhóm (Team Leader)**: `tit-coder` ([https://github.com/tit-coder](https://github.com/tit-coder))

---

## 1. Mục tiêu đồ án

1. **Hiểu sâu về Quản lý tiến trình trong Linux**: Cách hệ điều hành tạo tiến trình con (`fork()`), phân tách không gian bộ nhớ giữa tiến trình cha và con.
2. **Làm chủ Cơ chế Tín hiệu (POSIX Signals)**: Bản chất và hành vi của các tín hiệu `SIGSTOP`, `SIGCONT`, `SIGTERM`, `SIGKILL`, `SIGINT`.
3. **Phòng chống Zombie & Orphan Process**: Sử dụng hàm `waitpid()` kết hợp các cờ `WNOHANG`, `WUNTRACED`, `WCONTINUED` để thu hồi tài nguyên tiến trình con một cách phi phong tỏa (non-blocking).
4. **Khai thác Linux Virtual Filesystem (`/proc`)**: Đọc và bóc tách trực tiếp thông tin từ `/proc/<PID>/status` để hiển thị trạng thái thực tế của tiến trình trong kernel.
5. **Quy trình Phát triển Phần mềm & Git Chuẩn mực**: Phân chia kiến trúc mô-đun, làm việc nhóm theo mô hình Git Feature Branching và Pull Request.

---

## 2. Phân công nhiệm vụ & Phân nhánh Git (Team Git Workflow)

Dự án được xây dựng bởi nhóm 4 thành viên theo phân nhánh Git rõ ràng:

| Thành viên | Vai trò | Phụ trách kỹ thuật & File | Git Branch phụ trách |
| :--- | :--- | :--- | :--- |
| **Leader (`tit-coder`)** | Trưởng nhóm / Kiến trúc sư hệ thống | Điều phối kiến trúc tổng thể, `src/main.c`, tích hợp hệ thống, Code Review, Merge PRs và chuẩn bị Demo. | `main` |
| **Thành viên 2 (TV2)** | Process Management Engineer | Vòng đời tiến trình: `fork()`, worker loop, `waitpid()`, chống Zombie process trong `src/process_manager.c` và `src/process_manager.h`. | `feature/process-management` |
| **Thành viên 3 (TV3)** | Signal Handling Engineer | Gửi và xác thực tín hiệu: `kill()`, `SIGSTOP`, `SIGCONT`, `SIGTERM`, bắt `SIGINT` cha trong `src/signal_handler.c` và `src/signal_handler.h`. | `feature/signal-control` |
| **Thành viên 4 (TV4)** | Monitoring & QA Engineer | Phân tích `/proc/<PID>/status` trong `src/monitor.c`, `src/monitor.h`, xây dựng bộ test tự động trong thư mục `tests/` và `docs/test_cases.md`. | `feature/terminal-ui` |

---

## 3. Kiến trúc hệ thống & Cấu trúc thư mục

### 3.1. Sơ đồ tương tác giữa các mô-đun

```
                      +-----------------------------+
                      |         main.c              |  (Leader)
                      |  (CLI Coordinator & Menu)   |
                      +--------------+--------------+
                                     |
         +---------------------------+---------------------------+
         |                                                       |
         v                                                       v
+-------------------------------+                     +--------------------+
|     process_manager.c/.h      |                     |   monitor.c/.h     |
|   (TV2 - Process Lifecycle)   |                     | (TV4 - /proc info) |
| - fork() Worker               |                     | - Open /proc/PID   |
| - waitpid() (WNOHANG)         |                     | - Parse State/Mem  |
| - Zombie Elimination          |                     | - Explain State    |
+---------------+---------------+                     +---------+----------+
                |                                               |
                v                                               |
+-------------------------------+                               |
|     signal_handler.c/.h       |                               |
|   (TV3 - Signal Dispatcher)   |                               |
| - kill(SIGSTOP) -> Suspend    |                               |
| - kill(SIGCONT) -> Resume     |                               |
| - kill(SIGTERM) -> Terminate  |                               |
| - Parent SIGINT/SIGQUIT catch |                               |
+---------------+---------------+                               |
                |                                               |
                +-----------------------+-----------------------+
                                        |
                                        v
                         +-----------------------------+
                         |      Linux Kernel Space     |
                         |  (Process Table & /proc FS) |
                         +-----------------------------+
```

### 3.2. Cấu trúc thư mục chi tiết

```
linux-process-control/
├── .gitignore                  # Bỏ qua các file tạm, build artifacts, core dumps
├── Makefile                    # Makefile biên dịch nghiêm ngặt (-Wall -Wextra -Wpedantic)
├── README.md                   # Tài liệu hướng dẫn toàn diện dự án
├── docs/
│   ├── test_cases.md           # Bảng đặc tả 11 ca kiểm thử và kết quả thực tế
│   └── screenshots/            # Thư mục lưu trữ hình ảnh demo thực tế
│       └── .gitkeep
├── src/
│   ├── main.c                  # Hàm main, điều phối menu CLI tương tác và cờ test tự động
│   ├── process_manager.c       # Cài đặt tạo tiến trình, kiểm tra tồn tại và thu hồi zombie
│   ├── process_manager.h       # Khai báo nguyên mẫu hàm quản lý tiến trình
│   ├── signal_handler.c        # Cài đặt gửi tín hiệu POSIX và xác thực an toàn PID
│   ├── signal_handler.h        # Khai báo nguyên mẫu hàm gửi tín hiệu
│   ├── monitor.c               # Cài đặt đọc file /proc/<PID>/status và diễn giải trạng thái
│   └── monitor.h               # Khai báo nguyên mẫu hàm giám sát tiến trình
└── tests/
    ├── test_process.sh         # Script kiểm thử tạo tiến trình & chống zombie/orphan
    ├── test_signal.sh          # Script kiểm thử SIGSTOP (T), SIGCONT (S/R), SIGTERM
    └── test_invalid_pid.sh     # Script kiểm thử xác thực PID âm, PID chữ, PID 1
```

---

## 4. Bảng diễn giải trạng thái tiến trình Linux Kernel

Chương trình đọc trực tiếp trường `State:` từ file `/proc/<PID>/status` và diễn giải chính xác theo cơ chế của Linux kernel:

| Ký hiệu | Trạng thái (Linux Kernel State) | Diễn giải kỹ thuật chi tiết |
| :---: | :--- | :--- |
| **`T`** | **Stopped** | Tiến trình bị tạm dừng bởi Job Control Signal (như `SIGSTOP` hoặc `SIGTSTP`). Tiến trình ngừng thực thi hoàn toàn trên CPU cho đến khi nhận được `SIGCONT`. |
| **`t`** | **Tracing stop** | Tiến trình bị tạm dừng bởi trình gỡ lỗi (debugger) trong quá trình `ptrace`. |
| **`R`** | **Running / Runnable** | Đang thực thi trực tiếp trên CPU hoặc đang nằm trong hàng đợi sẵn sàng (`run-queue`) chờ bộ lập lịch (scheduler) cấp phát CPU slice. |
| **`S`** | **Interruptible Sleep** | Đang ngủ có thể ngắt (chờ một sự kiện, tín hiệu hoặc bộ đếm thời gian như `sleep()`). **Lưu ý quan trọng**: Trạng thái này không đồng nghĩa với việc đang ngốn CPU. |
| **`D`** | **Uninterruptible Sleep** | Đang ngủ sâu không thể ngắt bằng tín hiệu thông thường (thường do đang chờ I/O phần cứng/ổ đĩa hoàn thành). |
| **`Z`** | **Zombie** | Tiến trình con đã kết thúc thực thi nhưng tiến trình cha chưa gọi `wait()`/`waitpid()` để đọc mã thoát (`exit status`). Vẫn giữ một mục trong bảng tiến trình kernel (`process table`). |
| **`X`** | **Dead** | Tiến trình đã được giải phóng hoàn toàn khỏi bộ nhớ và bảng tiến trình. |

---

## 5. Hướng dẫn Biên dịch, Chạy và Kiểm thử

### 5.1. Yêu cầu môi trường
- Hệ điều hành: **Ubuntu Linux 20.04 / 22.04 / 24.04 LTS** (hoặc các bản phân phối Linux tương đương).
- Trình biên dịch: **GCC 9.0+** (đã kiểm thử trên GCC 13.3.0).
- Công cụ build: **GNU Make 4.0+**.
- Trình thông dịch shell: **Bash 4.0+**.

### 5.2. Các lệnh Make hỗ trợ

```bash
# 1. Biên dịch dự án (tạo thư mục build/ và bin/process_control)
make

# 2. Chạy chương trình tương tác trên Terminal
make run

# 3. Chạy toàn bộ bộ kiểm thử tích hợp tự động
make test

# 4. Xóa các file nhị phân và object files đã build
make clean
```

---

## 6. Hướng dẫn sử dụng Menu điều khiển

Khi chạy chương trình bằng `./bin/process_control` hoặc `make run`, hệ thống tự động `fork()` một tiến trình Worker con và hiển thị menu:

```
=================================================================
   LINUX PROCESS MANAGEMENT SYSTEM (C11 / POSIX.1-2008)          
   Subject: System Programming - HCMUT / Linux Dev Team          
   Team Leader: tit-coder | Repo: linux-process-control         
=================================================================

[Initiating] Creating child worker process via fork()...
[Process Manager] Spawned worker process PID: 5847
[Info] Worker process running with PID: 5847.

>>> [Worker PID 5847] Worker process started successfully (Parent PID: 5846).
>>> [Worker PID 5847] Incrementing counter every 1s. Suspend (SIGSTOP) will pause it.

[Worker PID 5847] [13:26:55] Heartbeat tick: count = 1

----------------------- [ CONTROL MENU ] ------------------------
 Active Worker PID: 5847 [ACTIVE]
-----------------------------------------------------------------
  1. Suspend Process   (Send SIGSTOP - Pause execution)
  2. Resume Process    (Send SIGCONT - Resume execution)
  3. Show Process Status (Inspect /proc/<PID>/status)
  4. Terminate Process (Send SIGTERM & reap child)
  5. Exit              (Safely terminate worker & quit)
-----------------------------------------------------------------
Enter your choice [1-5]: 
```

### Các chức năng chính:
- **Phím `1` (Suspend Process)**: Gửi tín hiệu `SIGSTOP`. Worker ngừng in bộ đếm ngay lập tức. Trạng thái trong `/proc/<PID>/status` chuyển sang `T (stopped)`.
- **Phím `2` (Resume Process)**: Gửi tín hiệu `SIGCONT`. Worker thức dậy và tiếp tục in bộ đếm. Trạng thái chuyển sang `S (sleeping)` hoặc `R (running)`.
- **Phím `3` (Show Process Status)**: Đọc file `/proc/<PID>/status` và xuất bảng chi tiết về PID, Name, PPid, State giải thích bằng tiếng Anh kỹ thuật, Virtual Memory (VmSize), RAM thực tế (VmRSS), số Threads.
- **Phím `4` (Terminate Process)**: Gửi tín hiệu `SIGTERM` yêu cầu Worker kết thúc nhẹ nhàng. Tiến trình cha lập tức gọi `waitpid()` để thu hồi, ngăn ngừa Zombie.
- **Phím `5` (Exit)**: Thu hồi Worker nếu vẫn đang hoạt động, giải phóng toàn bộ tài nguyên và thoát an toàn.

---

## 7. Quy trình Git & GitHub dành cho thành viên mới (Git Workflow)

Quy trình chuẩn mực giúp 4 thành viên phối hợp an toàn, không xung đột mã nguồn:

### Bước 1: Clone repository về máy tính
```bash
git clone https://github.com/tit-coder/linux-process-control.git
cd linux-process-control
```

### Bước 2: Chuyển sang branch tính năng được phân công
- Thành viên 2 (Process Management):
  ```bash
  git checkout feature/process-management
  ```
- Thành viên 3 (Signal Control):
  ```bash
  git checkout feature/signal-control
  ```
- Thành viên 4 (Terminal UI & Tests):
  ```bash
  git checkout feature/terminal-ui
  ```

### Bước 3: Thực hiện thay đổi, biên dịch và chạy kiểm thử
Trước khi commit, thành viên **bắt buộc** phải đảm bảo code không có warning và pass toàn bộ test:
```bash
make clean
make
make test
```

### Bước 4: Commit và Push code lên GitHub
```bash
# Kiểm tra file đã sửa
git status

# Đưa file vào staging area
git add src/process_manager.c src/process_manager.h

# Commit với thông điệp rõ ràng theo chuẩn Conventional Commits
git commit -m "feat(pm): implement worker spawning and non-blocking waitpid"

# Push lên branch tương ứng trên GitHub
git push origin feature/process-management
```

### Bước 5: Tạo Pull Request (PR) & Review Code
1. Vào trang GitHub [https://github.com/tit-coder/linux-process-control](https://github.com/tit-coder/linux-process-control).
2. Nhấn nút **Compare & pull request** từ branch của bạn vào branch `main`.
3. Gán Leader (`tit-coder`) vào mục **Reviewers**.
4. Leader kiểm tra diff, chạy thử nghiệm trên máy local, phê duyệt (Approve) và thực hiện **Merge pull request** vào `main`.

---

## 8. Kết quả kiểm thử tự động (Test Automation)

Cả 3 kịch bản kiểm thử trong thư mục `tests/` đều đã được chạy thực tế trên môi trường Ubuntu 24.04:

```
==================================================
Running Automated Integration Tests in tests/ ...
==================================================
[PASS] TEST 1: Process Creation & Zombie-free Cleanup
[PASS] TEST 2: Signal Control (SIGSTOP/CONT/TERM & State Transition)
[PASS] TEST 3: Invalid PID Rejection & Security Boundaries
[DONE] All test suites completed successfully.
```
Chi tiết từng bước kiểm thử xem tại [docs/test_cases.md](docs/test_cases.md).
