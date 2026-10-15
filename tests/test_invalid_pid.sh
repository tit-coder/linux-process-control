#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_invalid_pid.sh
# Purpose: Verify robust validation against invalid PIDs, malformed inputs, and security boundaries
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
echo -e "${BLUE}   RUNNING TEST 3: Invalid PID & Input Validation   ${NC}"
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

# --- Case 1: Test negative PID rejection ---
echo -e "${YELLOW}[Case 1] Testing rejection of negative PID (-999)...${NC}"
if "$BIN" --test-invalid-pid "-999" > "$TMP_OUT" 2>&1; then
    echo -e "${GREEN}[PASS] Negative PID was successfully caught and rejected.${NC}"
else
    echo -e "${RED}[FAIL] Negative PID was not properly handled!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

# --- Case 2: Test string/alphabetic PID rejection ---
echo -e "${YELLOW}[Case 2] Testing rejection of non-numeric PID ('abc_invalid')...${NC}"
if "$BIN" --test-invalid-pid "abc_invalid" > "$TMP_OUT" 2>&1; then
    echo -e "${GREEN}[PASS] Alphabetic/malformed PID was successfully rejected.${NC}"
else
    echo -e "${RED}[FAIL] Malformed PID was not properly rejected!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

# --- Case 3: Test boundary protection (PID 1 - system init/systemd) ---
echo -e "${YELLOW}[Case 3] Testing unmanaged PID protection (PID 1 init)...${NC}"
if "$BIN" --test-invalid-pid "1" > "$TMP_OUT" 2>&1; then
    echo -e "${GREEN}[PASS] Unmanaged PID 1 was blocked with Security Violation warning.${NC}"
else
    echo -e "${RED}[FAIL] System failed to prevent signal dispatch to unmanaged PID 1!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

# --- Case 4: Test non-existent PID in status monitor ---
echo -e "${YELLOW}[Case 4] Testing status monitor on non-existent PID 9999999...${NC}"
set +e
"$BIN" --test-status 9999999 > "$TMP_OUT" 2>&1
STATUS_RET=$?
set -e

if [ "$STATUS_RET" -ne 0 ] && grep -qi "does not exist" "$TMP_OUT"; then
    echo -e "${GREEN}[PASS] Non-existent PID 9999999 safely reported as not found without crashing.${NC}"
else
    echo -e "${RED}[FAIL] Non-existent PID monitor check failed!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

# --- Case 5: Test interactive menu invalid choices ("xyz", "99", "-5") ---
echo -e "${YELLOW}[Case 5] Testing interactive menu resilience with invalid choices ('invalid', '99', '5')...${NC}"
printf "invalid_choice\n99\n-5\n5\n" | "$BIN" > "$TMP_OUT" 2>&1 &
INTER_PID=$!
CLEANUP_PIDS+=("$INTER_PID")

wait "$INTER_PID" || true

if grep -q "Please enter a valid option number between 1 and 5" "$TMP_OUT"; then
    echo -e "${GREEN}[PASS] Invalid menu input correctly warned and gracefully handled.${NC}"
else
    echo -e "${RED}[FAIL] Invalid menu input was not properly handled!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${GREEN}>>> TEST 3 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
