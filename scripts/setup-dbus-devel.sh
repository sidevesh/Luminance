#!/bin/bash
# setup-dbus-devel.sh — install the Devel D-Bus service file and optionally
# patch the GNOME extension to talk to the Devel binary instead of production.
#
# BACKGROUND
# ----------
# The debug binary registers:   com.sidevesh.Luminance.Devel
#                   at path:   /com/sidevesh/Luminance/Devel/Service
#
# The installed extension uses: com.sidevesh.Luminance (production)
#                   at path:   /com/sidevesh/Luminance/Service
#
# This means two things are required for full extension+ghost testing:
#   1. A D-Bus service file so D-Bus can auto-start the Devel ghost when needed.
#   2. The extension's constants patched to the Devel names.
#
# USAGE
# -----
#   ./scripts/setup-dbus-devel.sh           # service file only
#   ./scripts/setup-dbus-devel.sh --patch-extension  # service file + patch extension
#   ./scripts/setup-dbus-devel.sh --restore          # undo extension patch only
#   ./scripts/setup-dbus-devel.sh --status           # show current state
#
# Run from the repo root.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="$REPO_ROOT/build/debug/com.sidevesh.Luminance.Devel"
SCHEMA_DIR="$REPO_ROOT/build/debug"

DEVEL_SERVICE_NAME="com.sidevesh.Luminance.Devel"
DBUS_USER_SERVICES_DIR="$HOME/.local/share/dbus-1/services"
SERVICE_FILE="$DBUS_USER_SERVICES_DIR/$DEVEL_SERVICE_NAME.service"

EXTENSION_JS="$REPO_ROOT/gnome-extension/luminance-extension@sidevesh/extension.js"
EXTENSION_BACKUP="$EXTENSION_JS.prod-backup"

MODE="${1:-}"

# ─── helpers ─────────────────────────────────────────────────────────────────
info()  { echo "INFO: $*"; }
ok()    { echo "OK:   $*"; }
warn()  { echo "WARN: $*"; }
err()   { echo "ERR:  $*" >&2; exit 1; }

# ─── --status ────────────────────────────────────────────────────────────────
if [ "$MODE" = "--status" ]; then
    echo "=== D-Bus service file ==="
    if [ -f "$SERVICE_FILE" ]; then
        echo "Installed: $SERVICE_FILE"
        grep Exec "$SERVICE_FILE"
    else
        echo "Not installed"
    fi
    echo ""
    echo "=== Extension constants ==="
    grep -E "^const (IFACE_NAME|BUS_NAME|OBJECT_PATH)" "$EXTENSION_JS" || true
    if [ -f "$EXTENSION_BACKUP" ]; then
        echo "(production backup exists at $EXTENSION_BACKUP)"
    fi
    exit 0
fi

# ─── --restore ───────────────────────────────────────────────────────────────
if [ "$MODE" = "--restore" ]; then
    if [ -f "$EXTENSION_BACKUP" ]; then
        cp "$EXTENSION_BACKUP" "$EXTENSION_JS"
        rm "$EXTENSION_BACKUP"
        ok "Extension restored to production constants"
        info "Reload the extension: gnome-extensions disable luminance-extension@sidevesh && gnome-extensions enable luminance-extension@sidevesh"
    else
        warn "No backup found — extension may already be at production constants"
    fi
    exit 0
fi

# ─── install service file ─────────────────────────────────────────────────────
if [ ! -x "$BINARY" ]; then
    err "Binary not found: $BINARY — run: ninja -C build/debug"
fi

mkdir -p "$DBUS_USER_SERVICES_DIR"

WRAPPER_SCRIPT="$REPO_ROOT/scripts/luminance-devel-ghost.sh"
if [ ! -x "$WRAPPER_SCRIPT" ]; then
    err "Wrapper script not found: $WRAPPER_SCRIPT"
fi

cat > "$SERVICE_FILE" <<EOF
[D-BUS Service]
Name=$DEVEL_SERVICE_NAME
Exec=$WRAPPER_SCRIPT --gapplication-service
EOF

ok "Service file installed: $SERVICE_FILE"
info "D-Bus will now auto-start the Devel ghost when something calls $DEVEL_SERVICE_NAME"
info "This covers: test-binary.sh, and the extension when patched with --patch-extension"

# ─── --patch-extension ────────────────────────────────────────────────────────
if [ "$MODE" = "--patch-extension" ]; then
    if [ -f "$EXTENSION_BACKUP" ]; then
        warn "Extension backup already exists — skipping patch (already patched?)"
        warn "Run --restore first if you want to re-patch"
        exit 0
    fi

    # Backup the production extension
    cp "$EXTENSION_JS" "$EXTENSION_BACKUP"

    # Patch the three constants at the top of extension.js
    sed -i \
        -e "s|^const IFACE_NAME = 'com\.sidevesh\.Luminance';|const IFACE_NAME = 'com.sidevesh.Luminance.Devel';|" \
        -e "s|^const BUS_NAME = 'com\.sidevesh\.Luminance';|const BUS_NAME = 'com.sidevesh.Luminance.Devel';|" \
        -e "s|^const OBJECT_PATH = '/com/sidevesh/Luminance/Service';|const OBJECT_PATH = '/com/sidevesh/Luminance/Devel/Service';|" \
        "$EXTENSION_JS"

    ok "Extension patched to Devel constants"
    info "Backup saved at: $EXTENSION_BACKUP"
    info "Reload the extension to apply:"
    info "  gnome-extensions disable luminance-extension@sidevesh && gnome-extensions enable luminance-extension@sidevesh"
    info "Or: Alt+F2 → 'r' → Enter  (X11 only)"
    echo ""
    info "When done testing, restore with: ./scripts/setup-dbus-devel.sh --restore"
fi
