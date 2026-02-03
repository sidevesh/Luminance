#!/bin/bash

BUS_NAME="com.sidevesh.Luminance"
OBJECT_PATH="/com/sidevesh/Luminance/Service"
INTERFACE="com.sidevesh.Luminance"

if [[ "$1" == "--devel" ]]; then
    BUS_NAME="com.sidevesh.Luminance.Devel"
    OBJECT_PATH="/com/sidevesh/Luminance/Devel/Service"
    echo "Running in development mode..."
fi

echo "Testing DBus Interface for $BUS_NAME..."

# Check if gdbus is installed
if ! command -v gdbus &> /dev/null; then
    echo "Error: gdbus tool is required but not installed."
    exit 1
fi

# 1. Get Monitors
echo -e "\n1. Calling GetMonitors..."
OUTPUT=$(gdbus call --session --dest $BUS_NAME --object-path $OBJECT_PATH --method $INTERFACE.GetMonitors 2>&1)

if [[ $? -ne 0 ]]; then
    echo "Error calling GetMonitors: $OUTPUT"
    echo "Ensure the application is running."
    exit 1
fi

echo "Raw Output: $OUTPUT"

# Extract JSON string roughly (gdbus returns ('json',))
# This sed command attempts to strip the wrapper to get the inner string
JSON_STR=$(echo "$OUTPUT" | sed "s/^('//; s/',)$//")
# Unescape quotes if needed (gdbus output might be escaped)
echo "JSON: $JSON_STR"

# Check if we have monitors (simple string check for "id")
# We invoke on monitor "0" by default if present
TARGET_MONITOR="0"

# 2. Monitor Signal
echo -e "\n2. Listening for ShowOSD signal in background..."
TMP_LOG=$(mktemp)
# Start monitoring in background, redirect to a temp file
gdbus monitor --session --dest $BUS_NAME --object-path $OBJECT_PATH > "$TMP_LOG" &
MONITOR_PID=$!

# Give it a moment to start
sleep 1

# 3. Set Brightness
# We'll try to set the brightness of monitor 0 to 50%
# Note: This might change your actual screen brightness!
echo -e "\n3. Calling SetBrightness('$TARGET_MONITOR', 50.0)..."
gdbus call --session --dest $BUS_NAME --object-path $OBJECT_PATH --method $INTERFACE.SetBrightness "$TARGET_MONITOR" 50.0

# Wait for signal capture
sleep 2

# Kill monitor process
kill $MONITOR_PID 2>/dev/null

# 4. Verification
echo -e "\n4. Verifying Signal..."
if grep -q "ShowOSD" "$TMP_LOG"; then
    echo -e "\033[0;32mSUCCESS: ShowOSD signal received.\033[0m"
    grep "ShowOSD" "$TMP_LOG" -A 1
else
    echo -e "\033[0;31mFAILURE: ShowOSD signal NOT received.\033[0m"
    echo "Log content:"
    cat "$TMP_LOG"
fi

rm "$TMP_LOG"
echo -e "\nTest Complete."
