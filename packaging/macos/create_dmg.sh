#!/usr/bin/env bash
set -euo pipefail

# Script to create and optionally sign a DMG file
# Usage: create_dmg.sh <app_path> <dmg_path> <app_sign_identity> <dmg_sign_identity>
# If signing identities are empty, DMG will be created unsigned

if [ $# -ne 4 ]; then
    echo "Usage: create_dmg.sh <app_path> <dmg_path> <app_sign_identity> <dmg_sign_identity>"
    echo "Note: Leave signing identities empty to create unsigned DMG"
    exit 1
fi

APP_PATH="$1"
DMG_PATH="$2"
APP_SIGN_IDENTITY="$3"
DMG_SIGN_IDENTITY="$4"

if [ ! -d "$APP_PATH" ]; then
    echo "App bundle not found: $APP_PATH" >&2
    exit 1
fi

if [ -n "$APP_SIGN_IDENTITY" ]; then
    codesign --verify --deep --strict --verbose=2 "$APP_PATH"
fi

echo "Creating DMG from: $APP_PATH"
echo "Output DMG: $DMG_PATH"

rm -f "$DMG_PATH"

TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT
echo "Using temp directory: $TEMP_DIR"

cp -R "$APP_PATH" "$TEMP_DIR/"

# Create Applications symlink
ln -s /Applications "$TEMP_DIR/Applications"

# Calculate size needed for DMG (in KB)
SIZE=$(du -sk "$TEMP_DIR" | cut -f1)
SIZE=$((SIZE + 1000))  # Add some padding

echo "Creating DMG with size: ${SIZE}k"

# Create DMG
hdiutil create -srcfolder "$TEMP_DIR" \
    -volname "LinScreen" \
    -fs HFS+ \
    -fsargs "-c c=64,a=16,e=16" \
    -format UDZO \
    -size "${SIZE}k" \
    "$DMG_PATH"

echo "DMG created successfully"

# Sign the DMG (either with identity or ad hoc)
if [ -n "$DMG_SIGN_IDENTITY" ] && [ "$DMG_SIGN_IDENTITY" != "" ]; then
    echo "Signing DMG with identity: $DMG_SIGN_IDENTITY"
    codesign --force --sign "$DMG_SIGN_IDENTITY" --timestamp "$DMG_PATH"
    echo "DMG signed with Developer ID"
    codesign --verify --strict --verbose=2 "$DMG_PATH"
else
    echo "Signing DMG with ad hoc signature (no identity required)"
    codesign --force --sign - "$DMG_PATH"
    echo "DMG signed with ad hoc signature"
fi

echo "DMG creation complete: $DMG_PATH"

if [ -z "$DMG_SIGN_IDENTITY" ] || [ "$DMG_SIGN_IDENTITY" = "" ]; then
    echo ""
    echo "NOTE: This DMG uses ad hoc signing (no Developer ID required)."
    echo "This development artifact is not suitable for public distribution."
fi
