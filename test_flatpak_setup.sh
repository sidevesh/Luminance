#!/bin/bash

echo "Testing Luminance Flatpak Setup Configuration..."

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_file() {
    if [ -f "$1" ]; then
        echo -e "[${GREEN}OK${NC}] File exists: $1"
    else
        echo -e "[${RED}FAIL${NC}] File missing: $1"
    fi
}

check_module() {
    if lsmod | grep "$1" > /dev/null; then
        echo -e "[${GREEN}OK${NC}] Kernel module loaded: $1"
    else
        echo -e "[${RED}FAIL${NC}] Kernel module NOT loaded: $1"
    fi
}

echo "--- Checking Installation Files ---"
check_file "/usr/lib/udev/rules.d/60-ddcutil-i2c.rules"
check_file "/usr/lib/udev/rules.d/44-backlight-permissions.rules"
check_file "/usr/lib/modules-load.d/ddcutil.conf"

echo ""
echo "--- Checking Kernel Modules ---"
check_module "i2c_dev"

echo ""
echo "--- Checking I2C Devices ---"
I2C_COUNT=$(ls /dev/i2c-* 2>/dev/null | wc -l)
if [ "$I2C_COUNT" -gt 0 ]; then
    echo -e "[${GREEN}OK${NC}] Found $I2C_COUNT i2c devices in /dev/"
else
    echo -e "[${YELLOW}WARN${NC}] No /dev/i2c-* devices found. DDC/CI might not work."
fi

echo ""
echo "--- Checking Backlight Interfaces ---"
if [ -d "/sys/class/backlight" ]; then
    BACKLIGHT_COUNT=$(ls /sys/class/backlight/ | wc -l)
    if [ "$BACKLIGHT_COUNT" -gt 0 ]; then
        echo -e "[${GREEN}OK${NC}] Found $BACKLIGHT_COUNT backlight interfaces in /sys/class/backlight/"
    else
        echo -e "[${YELLOW}INFO${NC}] No internal backlight interfaces found (normal for desktops)"
    fi
else
    echo -e "[${YELLOW}INFO${NC}] /sys/class/backlight directory does not exist"
fi

echo ""
echo "Done."
