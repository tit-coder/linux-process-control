# ==============================================================================
# Makefile for Linux Process Management System
# Project: linux-process-control
# Author: Linux System Programming Team (Leader: tit-coder)
# ==============================================================================

CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Isrc -g
LDFLAGS :=

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
TARGET := $(BIN_DIR)/process_control

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all run clean test help

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "[LINK] $@"
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC]   $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

run: $(TARGET)
	@echo "[RUN] Executing $(TARGET)..."
	./$(TARGET)

test: $(TARGET)
	@echo "=================================================="
	@echo "Running Automated Integration Tests in tests/ ..."
	@echo "=================================================="
	@chmod +x tests/*.sh
	@./tests/test_process.sh
	@./tests/test_signal.sh
	@./tests/test_invalid_pid.sh
	@echo "[DONE] All test suites completed."

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

help:
	@echo "Usage:"
	@echo "  make         - Compile the project (creates $(TARGET))"
	@echo "  make run     - Run the interactive process control system"
	@echo "  make test    - Run all automated test scripts"
	@echo "  make clean   - Remove object files and executables"
