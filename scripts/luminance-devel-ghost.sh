#!/bin/bash
# Wrapper for D-Bus service activation of the Devel ghost.
# Sets GSETTINGS_SCHEMA_DIR so the debug binary can find its compiled gschema.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
exec env GSETTINGS_SCHEMA_DIR="$REPO_ROOT/build/debug" \
  "$REPO_ROOT/build/debug/com.sidevesh.Luminance.Devel" "$@"
