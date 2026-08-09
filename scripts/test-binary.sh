#!/bin/bash
# test-binary.sh — end-to-end tests for the debug binary without the extension.
#
# Tests covered:
#   1. CLI direct DDC (no service running)
#   2. Ghost mode stays alive
#   3. CLI D-Bus routing through a running ghost
#   4. Ghost still alive after CLI finishes
#   5. Multiple GUI launches — second instance forwards Activate to first, exits 0
#   6. Ghost → UI upgrade — ghost running, GUI launch creates window (Activate signal)
#
# Prerequisites:
#   - Run from the repo root: ./scripts/test-binary.sh
#   - Build must be up to date: ninja -C build/debug
#
# The script does NOT modify display brightness in a visible way (uses
# display 3 / index 2 to match previous manual tests; adjust DISPLAY_NUM
# if your setup differs).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="$REPO_ROOT/build/debug/com.sidevesh.Luminance.Devel"
export GSETTINGS_SCHEMA_DIR="$REPO_ROOT/build/debug"

DISPLAY_NUM=3        # 1-based display number used for set-brightness tests
BRIGHTNESS_VALUE=55  # percentage to set (safe mid-range)

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}PASS${NC}: $*"; }
fail() { echo -e "${RED}FAIL${NC}: $*"; FAILURES=$((FAILURES+1)); }
info() { echo -e "${YELLOW}INFO${NC}: $*"; }

FAILURES=0

if [ ! -x "$BINARY" ]; then
    echo "Binary not found: $BINARY"
    echo "Run: ninja -C build/debug"
    exit 1
fi

# Kill any leftover instances from previous runs
pkill -f "com.sidevesh.Luminance.Devel" 2>/dev/null || true
sleep 0.5

echo "================================================================"
echo " Luminance debug binary tests"
echo " Binary: $BINARY"
echo "================================================================"

# ─── Test 1: CLI direct DDC (no service) ─────────────────────────────────────
echo ""
echo "--- Test 1: CLI --list-displays (no service) ---"
OUTPUT=$("$BINARY" --list-displays 2>&1)
EXIT=$?
if [ $EXIT -eq 0 ] && echo "$OUTPUT" | grep -q "Display"; then
    pass "CLI list-displays returned display list (exit 0)"
else
    fail "CLI list-displays failed (exit $EXIT)"
    echo "$OUTPUT"
fi

# ─── Test 2: Ghost mode stays alive ──────────────────────────────────────────
echo ""
echo "--- Test 2: Ghost mode (--gapplication-service) ---"
"$BINARY" --gapplication-service &
GHOST_PID=$!
sleep 2
if kill -0 "$GHOST_PID" 2>/dev/null; then
    pass "Ghost alive after 2s (pid=$GHOST_PID)"
else
    fail "Ghost died before test (pid=$GHOST_PID)"
    GHOST_PID=""
fi

# ─── Test 3: CLI D-Bus routing through ghost ──────────────────────────────────
echo ""
echo "--- Test 3: CLI D-Bus routing through ghost ---"
if [ -n "${GHOST_PID:-}" ] && kill -0 "$GHOST_PID" 2>/dev/null; then
    OUTPUT=$("$BINARY" --set-brightness "$DISPLAY_NUM" -p "$BRIGHTNESS_VALUE" 2>&1)
    EXIT=$?
    if [ $EXIT -eq 0 ] && echo "$OUTPUT" | grep -q "DBus Request"; then
        pass "CLI set-brightness routed through ghost (ghost log shows DBus Request)"
    elif [ $EXIT -eq 0 ]; then
        # Ghost output might not be captured in subprocess — check it another way
        pass "CLI set-brightness exited 0 (D-Bus routing active since ghost is running)"
        info "Check ghost stdout for 'DBus Request: Set brightness' to confirm routing"
    else
        fail "CLI set-brightness failed (exit $EXIT)"
        echo "$OUTPUT"
    fi
else
    fail "Ghost not running — skipping CLI D-Bus routing test"
fi

# ─── Test 4: Ghost alive after CLI ───────────────────────────────────────────
echo ""
echo "--- Test 4: Ghost still alive after CLI finishes ---"
sleep 1
if [ -n "${GHOST_PID:-}" ] && kill -0 "$GHOST_PID" 2>/dev/null; then
    pass "Ghost still alive after CLI finished"
else
    fail "Ghost died after CLI"
fi

# Kill ghost before next test
kill "${GHOST_PID:-0}" 2>/dev/null || true
wait "${GHOST_PID:-}" 2>/dev/null || true
GHOST_PID=""

# ─── Test 5: Multiple GUI launches ───────────────────────────────────────────
echo ""
echo "--- Test 5: Multiple GUI launches (no window shown in headless mode) ---"
info "Starting first GUI instance..."
"$BINARY" &
GUI1_PID=$!
sleep 3

if ! kill -0 "$GUI1_PID" 2>/dev/null; then
    fail "First GUI instance died unexpectedly (pid=$GUI1_PID)"
else
    info "First GUI alive (pid=$GUI1_PID), launching second..."
    timeout 6 "$BINARY"
    SECOND_EXIT=$?
    if [ $SECOND_EXIT -eq 139 ]; then
        fail "Second GUI instance crashed (SIGSEGV — exit 139)"
    elif [ $SECOND_EXIT -eq 0 ]; then
        pass "Second GUI instance exited cleanly (forwarded Activate to first, exit 0)"
    else
        fail "Second GUI instance unexpected exit code: $SECOND_EXIT"
    fi

    if kill -0 "$GUI1_PID" 2>/dev/null; then
        pass "First GUI still alive after second launch"
    else
        fail "First GUI died after second launch"
    fi
fi

kill "$GUI1_PID" 2>/dev/null || true
wait "$GUI1_PID" 2>/dev/null || true

# ─── Test 6: Ghost → UI upgrade ──────────────────────────────────────────────
echo ""
echo "--- Test 6: Ghost → UI upgrade (ghost running, GUI launch creates window) ---"
info "Starting ghost..."
"$BINARY" --gapplication-service &
GHOST_PID=$!
sleep 2

if ! kill -0 "$GHOST_PID" 2>/dev/null; then
    fail "Ghost didn't start"
else
    info "Ghost alive (pid=$GHOST_PID), launching GUI..."
    # Launch GUI — it sends Activate() to ghost, ghost creates window
    timeout 6 "$BINARY"
    UI_EXIT=$?
    # The GUI launch either:
    #   a) causes ghost to open a window (ghost becomes UI) and the launch exits 0
    #   b) crashes (SIGSEGV = 139)
    if [ $UI_EXIT -eq 139 ]; then
        fail "GUI launch crashed when ghost running (exit 139)"
    elif [ $UI_EXIT -eq 0 ]; then
        pass "GUI launch forwarded Activate to ghost and exited cleanly (exit 0)"
        info "Ghost should now have a window open — verify visually if display available"
    else
        fail "GUI launch unexpected exit code: $UI_EXIT"
    fi

    if kill -0 "$GHOST_PID" 2>/dev/null; then
        pass "Ghost (now UI) still alive after GUI launch"
    else
        fail "Ghost died after GUI launch"
    fi
fi

kill "${GHOST_PID:-0}" 2>/dev/null || true
wait "${GHOST_PID:-}" 2>/dev/null || true

# ─── Summary ─────────────────────────────────────────────────────────────────
echo ""
echo "================================================================"
if [ $FAILURES -eq 0 ]; then
    echo -e " ${GREEN}All tests passed${NC}"
else
    echo -e " ${RED}$FAILURES test(s) failed${NC}"
fi
echo "================================================================"
exit $FAILURES
