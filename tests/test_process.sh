#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_process.sh
# Purpose: Verify worker process creation via fork(), PID tracking, and zombie-free termination
# Author: TV4 & Linux System Programming Team
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${PROJECT_ROOT}/bin/process_control"

# ANSI Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}====================================================${NC}"
echo -e "${BLUE}   RUNNING TEST 1: Process Creation & Cleanup       ${NC}"
echo -e "${BLUE}====================================================${NC}"

if [ ! -x "$BIN" ]; then
    echo -e "${RED}[FAIL] Executable $BIN not found or not executable. Run 'make' first.${NC}"
    exit 1
fi

TMP_OUT=$(mktemp)
CLEANUP_PIDS=()

cleanup() {
    rm -f "$TMP_OUT"
    for p in "${CLEANUP_PIDS[@]}"; do
        if kill -0 "$p" 2>/dev/null; then
            kill -9 "$p" 2>/dev/null || true
        fi
    done
}
trap cleanup EXIT

echo -e "${YELLOW}[Step 1] Starting process_control with pipe input to create worker...${NC}"
# Feed menu command: 3 (Status), then 5 (Exit)
printf "3\n5\n" | "$BIN" > "$TMP_OUT" 2>&1 &
PARENT_PID=$!
CLEANUP_PIDS+=("$PARENT_PID")

# Wait for execution to finish
wait "$PARENT_PID" || true

# Extract Worker PID from output
WORKER_PID=$(grep -oE "Spawned worker process PID: [0-9]+" "$TMP_OUT" | awk '{print $NF}' | head -n 1 || true)

if [ -z "$WORKER_PID" ]; then
    echo -e "${RED}[FAIL] Could not detect spawned Worker PID in output!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${GREEN}[PASS] Worker process successfully created via fork() with PID: ${WORKER_PID}.${NC}"

echo -e "${YELLOW}[Step 2] Verifying Worker process was cleaned up upon program exit...${NC}"
# Allow kernel to finalize process table
sleep 0.5

if [ -d "/proc/${WORKER_PID}" ]; then
    STATE=$(awk '/State:/ {print $2}' "/proc/${WORKER_PID}/status" 2>/dev/null || echo "DEAD")
    if [ "$STATE" == "Z" ]; then
        echo -e "${RED}[FAIL] Worker PID ${WORKER_PID} is a ZOMBIE process! waitpid() was not called properly.${NC}"
        exit 1
    elif [ "$STATE" != "DEAD" ]; then
        echo -e "${RED}[FAIL] Worker PID ${WORKER_PID} is still alive (State: $STATE) after parent exit! Orphan detected.${NC}"
        exit 1
    fi
fi

# Also check with kill(pid, 0)
if kill -0 "$WORKER_PID" 2>/dev/null; then
    echo -e "${RED}[FAIL] Worker PID ${WORKER_PID} is still responding to signals after exit!${NC}"
    exit 1
fi

echo -e "${GREEN}[PASS] Worker process ${WORKER_PID} terminated cleanly. No Zombie or Orphan detected!${NC}"
echo -e "${GREEN}>>> TEST 1 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
