#! /bin/bash

# Function to handle uninstallation
uninstall() {
    echo "Uninstalling setup for Luminance..."
    
    # Remove files
    if [ -f "/usr/lib/udev/rules.d/60-ddcutil-i2c.rules" ]; then
        echo "Removing /usr/lib/udev/rules.d/60-ddcutil-i2c.rules..."
        sudo rm /usr/lib/udev/rules.d/60-ddcutil-i2c.rules
    fi
    
    if [ -f "/usr/lib/udev/rules.d/44-backlight-permissions.rules" ]; then
         echo "Removing /usr/lib/udev/rules.d/44-backlight-permissions.rules..."
         sudo rm /usr/lib/udev/rules.d/44-backlight-permissions.rules
    fi

    if [ -f "/usr/lib/modules-load.d/ddcutil.conf" ]; then
        echo "Removing /usr/lib/modules-load.d/ddcutil.conf..."
        sudo rm /usr/lib/modules-load.d/ddcutil.conf
    fi

    echo "Reloading udev rules..."
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    
    echo "Uninstallation complete."
    exit 0
}

# Function to verify installation
verify() {
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
    echo "Verification complete."
    exit 0
}

# Check for uninstall flag
if [ "$1" == "--uninstall" ]; then
    uninstall
fi

# Check for verify flag
if [ "$1" == "--verify" ]; then
    verify
fi

echo "Starting setup for Luminance..."

STEAMOS=0
STEAMOS_READONLY=0

# Test for SteamOS and disable readonly mode if we're running on it
if command -v steamos-readonly >& /dev/null
then
	# Test if SteamOS readonly mode is enabled
	if sudo steamos-readonly status | grep 'enabled'
	then
		echo "SteamOS readonly mode is enabled. Disabling temporarily..."
		STEAMOS_READONLY=1
	fi

	STEAMOS=1
	sudo steamos-readonly disable
fi

echo "Downloading ddcutil rules..."
wget https://raw.githubusercontent.com/rockowitz/ddcutil/v2.1.4/data/usr/lib/udev/rules.d/60-ddcutil-i2c.rules

echo "Downloading ddcutil module config..."
wget https://raw.githubusercontent.com/rockowitz/ddcutil/v2.1.4/data/usr/lib/modules-load.d/ddcutil.conf

echo "Downloading Luminance backlight permissions..."
wget https://raw.githubusercontent.com/sidevesh/Luminance/main/install_files/44-backlight-permissions.rules

echo "Moving files to system directories..."

# Move udev rules files to udev rules directory
echo "Installing udev rules..."
sudo mv 60-ddcutil-i2c.rules /usr/lib/udev/rules.d/
sudo mv 44-backlight-permissions.rules /usr/lib/udev/rules.d/

# Move ddcutil modules-load config
echo "Installing module load config..."
sudo mkdir -p /usr/lib/modules-load.d
sudo mv ddcutil.conf /usr/lib/modules-load.d/

# Reload the rules
echo "Reloading udev rules..."
sudo udevadm control --reload-rules
sudo udevadm trigger

# Load i2c-dev module immediately
echo "Loading i2c-dev module..."
sudo modprobe i2c-dev

if [ "$STEAMOS" = 1 ] ; then
	if [ "$STEAMOS_READONLY" = 1 ] ; then
		echo "Re-enabling SteamOS readonly mode..."
		sudo steamos-readonly enable
	fi
fi

echo "Setup complete!"
echo "To verify the installation, run this script with the --verify flag."
echo "./flatpak-setup.sh --verify"
echo "To uninstall these changes later, run this script with the --uninstall flag."
echo "./flatpak-setup.sh --uninstall"
