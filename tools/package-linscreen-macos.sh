#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-linscreen-macos}"
PHASE="${2:-all}"
SIGN_IDENTITY="${MACOS_SIGN_IDENTITY:-}"
NOTARY_KEY="${MACOS_NOTARY_KEY:-}"
NOTARY_KEY_ID="${MACOS_NOTARY_KEY_ID:-}"
NOTARY_ISSUER_ID="${MACOS_NOTARY_ISSUER_ID:-}"
REQUIRE_SIGNING="${REQUIRE_SIGNING:-0}"

if [ "$(uname -s)" != "Darwin" ]; then
    echo "This packaging script must run on macOS." >&2
    exit 1
fi

for command in cmake codesign hdiutil xcrun; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "$command is required to package LinScreen for macOS." >&2
        exit 1
    fi
done

if [ "$REQUIRE_SIGNING" = "1" ]; then
    if [ -z "$SIGN_IDENTITY" ] || [ -z "$NOTARY_KEY" ] || \
       [ -z "$NOTARY_KEY_ID" ] || [ -z "$NOTARY_ISSUER_ID" ]; then
        echo "Developer ID signing and App Store Connect notarization credentials are required for a release build." >&2
        exit 1
    fi
    if [ ! -f "$NOTARY_KEY" ]; then
        echo "The App Store Connect API key file was not found." >&2
        exit 1
    fi
    if ! security find-identity -v -p codesigning | grep -F -- "$SIGN_IDENTITY" >/dev/null; then
        echo "The requested Developer ID signing identity is not available in the keychain." >&2
        exit 1
    fi
fi

cmake_args=(
    -S .
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_TESTING=OFF
    -DENABLE_IMGUR=OFF
    -DCODE_SIGN_IDENTITY="$SIGN_IDENTITY"
    -DDMG_SIGN_IDENTITY="$SIGN_IDENTITY"
)

if [ -n "${Qt6_DIR:-}" ]; then
    cmake_args+=("-DQt6_DIR=${Qt6_DIR}")
elif [ -n "${QT_ROOT_DIR:-}" ]; then
    cmake_args+=("-DCMAKE_PREFIX_PATH=${QT_ROOT_DIR}")
fi

configure_build() {
    echo "[$(date -u +%H:%M:%S)] Configuring the macOS build"
    cmake "${cmake_args[@]}"
}

build_app() {
    echo "[$(date -u +%H:%M:%S)] Building the macOS app"
    cmake --build "$BUILD_DIR" --config Release --target linscreen --parallel
}

package_dmg() {
    echo "[$(date -u +%H:%M:%S)] Creating and verifying the DMG"
    cmake --build "$BUILD_DIR" --config Release --target create_dmg

    APP_PATH="$BUILD_DIR/src/LinScreen.app"
    DMG_PATH=$(find "$BUILD_DIR/src" -maxdepth 1 -type f -name 'LinScreen-*.dmg' -print -quit)

    if [ ! -d "$APP_PATH" ] || [ -z "$DMG_PATH" ]; then
        echo "The macOS app bundle or DMG was not generated." >&2
        exit 1
    fi

    codesign --verify --deep --strict --verbose=2 "$APP_PATH"
    codesign --verify --strict --verbose=2 "$DMG_PATH"

    if [ "$REQUIRE_SIGNING" = "1" ]; then
        xcrun notarytool submit "$DMG_PATH" \
            --key "$NOTARY_KEY" \
            --key-id "$NOTARY_KEY_ID" \
            --issuer "$NOTARY_ISSUER_ID" \
            --wait
        xcrun stapler staple "$DMG_PATH"
        xcrun stapler validate "$DMG_PATH"
        spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_PATH"
    else
        echo "Warning: ad hoc development build; Gatekeeper will reject it for public distribution." >&2
    fi

    echo "macOS package written to $DMG_PATH"
}

case "$PHASE" in
    configure) configure_build ;;
    build) build_app ;;
    package) package_dmg ;;
    all)
        configure_build
        build_app
        package_dmg
        ;;
    *)
        echo "Unknown phase '$PHASE'. Use configure, build, package, or all." >&2
        exit 1
        ;;
esac
