#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_signal.sh
# Purpose: Verify SIGSTOP (Suspend), SIGCONT (Resume), and SIGTERM (Terminate)
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
echo -e "${BLUE}   RUNNING TEST 2: Signal Control (SIGSTOP/CONT/TERM)${NC}"
echo -e "${BLUE}====================================================${NC}"

if [ ! -x "$BIN" ]; then
    echo -e "${RED}[FAIL] Executable $BIN not found or not executable. Run 'make' first.${NC}"
    exit 1
fi

TMP_FIFO=$(mktemp -u)
mkfifo "$TMP_FIFO"
TMP_OUT=$(mktemp)

CLEANUP_PIDS=()
cleanup() {
    rm -f "$TMP_FIFO" "$TMP_OUT"
    for p in "${CLEANUP_PIDS[@]}"; do
        if kill -0 "$p" 2>/dev/null; then
            kill -9 "$p" 2>/dev/null || true
        fi
    done
}
trap cleanup EXIT

# Launch process_control with interactive FIFO
tail -f "$TMP_FIFO" | "$BIN" > "$TMP_OUT" 2>&1 &
MAIN_PID=$!
CLEANUP_PIDS+=("$MAIN_PID")

# Wait for worker process to spawn
WORKER_PID=""
for i in {1..20}; do
    WORKER_PID=$(grep -oE "Spawned worker process PID: [0-9]+" "$TMP_OUT" | awk '{print $NF}' | head -n 1 || true)
    if [ -n "$WORKER_PID" ]; then
        CLEANUP_PIDS+=("$WORKER_PID")
        break
    fi
    sleep 0.2
done

if [ -z "$WORKER_PID" ]; then
    echo -e "${RED}[FAIL] Timed out waiting for Worker PID to initialize!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${GREEN}[INFO] Worker running with PID: ${WORKER_PID}${NC}"

# Give worker 1s to run
sleep 1.2

# --- Test 1: Suspend (Menu option 1 -> SIGSTOP) ---
echo -e "${YELLOW}[Step 1] Sending Menu 1 (Suspend - SIGSTOP)...${NC}"
echo "1" > "$TMP_FIFO"
sleep 0.8

if [ ! -d "/proc/${WORKER_PID}" ]; then
    echo -e "${RED}[FAIL] Worker process disappeared unexpectedly during suspend!${NC}"
    exit 1
fi

STATE_SUSPEND=$(awk '/State:/ {print $2}' "/proc/${WORKER_PID}/status")
echo -e "         Worker State in /proc/${WORKER_PID}/status: '${STATE_SUSPEND}'"

if [[ "$STATE_SUSPEND" == "T" || "$STATE_SUSPEND" == "t" ]]; then
    echo -e "${GREEN}[PASS] Worker process is correctly STOPPED (State: ${STATE_SUSPEND}) following SIGSTOP.${NC}"
else
    echo -e "${RED}[FAIL] Expected state 'T' or 't', but found '${STATE_SUSPEND}'!${NC}"
    exit 1
fi

# --- Test 2: Resume (Menu option 2 -> SIGCONT) ---
echo -e "${YELLOW}[Step 2] Sending Menu 2 (Resume - SIGCONT)...${NC}"
echo "2" > "$TMP_FIFO"
sleep 0.8

STATE_RESUME=$(awk '/State:/ {print $2}' "/proc/${WORKER_PID}/status")
echo -e "         Worker State in /proc/${WORKER_PID}/status: '${STATE_RESUME}'"

if [[ "$STATE_RESUME" == "S" || "$STATE_RESUME" == "R" ]]; then
    echo -e "${GREEN}[PASS] Worker process is correctly RESUMED (State: ${STATE_RESUME}) following SIGCONT.${NC}"
else
    echo -e "${RED}[FAIL] Expected state 'S' or 'R', but found '${STATE_RESUME}'!${NC}"
    exit 1
fi

# --- Test 3: Status inspection (Menu option 3 -> /proc reading) ---
echo -e "${YELLOW}[Step 3] Sending Menu 3 (Show Process Status)...${NC}"
echo "3" > "$TMP_FIFO"
sleep 0.5

if grep -q "PROCESS STATUS (/proc/${WORKER_PID}/status)" "$TMP_OUT"; then
    echo -e "${GREEN}[PASS] Status monitor successfully read and parsed /proc/${WORKER_PID}/status.${NC}"
else
    echo -e "${RED}[FAIL] Status monitor table not detected in output!${NC}"
    exit 1
fi

# --- Test 4: Terminate (Menu option 4 -> SIGTERM) ---
echo -e "${YELLOW}[Step 4] Sending Menu 4 (Terminate - SIGTERM)...${NC}"
echo "4" > "$TMP_FIFO"
sleep 1.0

if kill -0 "$WORKER_PID" 2>/dev/null; then
    echo -e "${RED}[FAIL] Worker PID ${WORKER_PID} is still alive after Terminate command!${NC}"
    exit 1
else
    echo -e "${GREEN}[PASS] Worker PID ${WORKER_PID} successfully terminated and reaped!${NC}"
fi

# Exit cleanly
echo "5" > "$TMP_FIFO"
sleep 0.5

echo -e "${GREEN}>>> TEST 2 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
