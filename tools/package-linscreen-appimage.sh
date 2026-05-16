#!/usr/bin/env sh
set -eu

build_dir="${1:-build-linscreen-appimage}"
tools_dir="${APPIMAGE_TOOLS_DIR:-${build_dir}/appimage-tools}"
appdir="${APPDIR:-${build_dir}/LinScreen.AppDir}"
project_version="$(sed -n 's/^set(LINSCREEN_VERSION[[:space:]]*\([0-9][^)]*\)).*/\1/p' CMakeLists.txt | head -n 1)"
arch="${ARCH:-x86_64}"
output_name="${OUTPUT_NAME:-LinScreen-${project_version}-${arch}.AppImage}"
qmake="${QMAKE:-qmake6}"

linuxdeploy_url="${LINUXDEPLOY_URL:-https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${arch}.AppImage}"
qt_plugin_url="${LINUXDEPLOY_QT_PLUGIN_URL:-https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${arch}.AppImage}"

if [ -z "${project_version}" ]; then
    echo "Could not read LINSCREEN_VERSION from CMakeLists.txt" >&2
    exit 1
fi

for command_name in cmake curl chmod find; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        echo "${command_name} is required to build the LinScreen AppImage." >&2
        exit 1
    fi
done

if ! command -v "${qmake}" >/dev/null 2>&1; then
    echo "${qmake} is required to build the LinScreen AppImage." >&2
    exit 1
fi

qt_plugins_dir="${QT_PLUGINS_DIR:-$("${qmake}" -query QT_INSTALL_PLUGINS 2>/dev/null || true)}"
qt_translations_dir="${QT_TRANSLATIONS_DIR:-$("${qmake}" -query QT_INSTALL_TRANSLATIONS 2>/dev/null || true)}"

case "${appdir}" in
    /*) appdir_abs="${appdir}" ;;
    *) appdir_abs="$(pwd)/${appdir}" ;;
esac

case "${build_dir}" in
    /*) output_abs="${build_dir}/${output_name}" ;;
    *) output_abs="$(pwd)/${build_dir}/${output_name}" ;;
esac

if [ "${arch}" != "x86_64" ]; then
    echo "This script currently downloads linuxdeploy tools for x86_64 only." >&2
    echo "Set LINUXDEPLOY_URL and LINUXDEPLOY_QT_PLUGIN_URL for ${arch}." >&2
    exit 1
fi

mkdir -p "${tools_dir}"
linuxdeploy="${tools_dir}/linuxdeploy"
qt_plugin="${tools_dir}/linuxdeploy-plugin-qt"

download_tool() {
    url="$1"
    destination="$2"

    if [ ! -x "${destination}" ]; then
        curl -L --fail --retry 3 -o "${destination}" "${url}"
        chmod +x "${destination}"
    fi
}

download_tool "${linuxdeploy_url}" "${linuxdeploy}"
download_tool "${qt_plugin_url}" "${qt_plugin}"

rm -rf "${appdir}" "${build_dir}/squashfs-root"

cmake -S . -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DUSE_LAUNCHER_ABSOLUTE_PATH=OFF \
    -DDISABLE_UPDATE_CHECKER=OFF \
    -DENABLE_IMGUR=OFF

cmake --build "${build_dir}" -j"$(nproc)"
DESTDIR="${appdir_abs}" cmake --install "${build_dir}"

desktop_file="${appdir}/usr/share/applications/org.linscreen.LinScreen.desktop"
icon_file="${appdir}/usr/share/icons/hicolor/scalable/apps/org.linscreen.LinScreen.svg"
metainfo_file="${appdir}/usr/share/metainfo/org.linscreen.LinScreen.metainfo.xml"
appdata_file="${appdir}/usr/share/metainfo/org.linscreen.LinScreen.appdata.xml"

rm -rf \
    "${appdir}/usr/include" \
    "${appdir}/usr/lib/x86_64-linux-gnu/cmake" \
    "${appdir}/usr/lib/x86_64-linux-gnu/pkgconfig"
find "${appdir}/usr/lib" -name '*.a' -delete

if [ -f "${metainfo_file}" ] && [ ! -f "${appdata_file}" ]; then
    cp "${metainfo_file}" "${appdata_file}"
fi

if [ -n "${qt_plugins_dir}" ] && [ -f "${qt_plugins_dir}/platforms/libqwayland.so" ]; then
    mkdir -p "${appdir}/usr/plugins/platforms"
    cp "${qt_plugins_dir}/platforms/libqwayland.so" "${appdir}/usr/plugins/platforms/"

    for plugin_dir in \
        wayland-decoration-client \
        wayland-graphics-integration-client \
        wayland-shell-integration; do
        if [ -d "${qt_plugins_dir}/${plugin_dir}" ]; then
            mkdir -p "${appdir}/usr/plugins/${plugin_dir}"
            cp "${qt_plugins_dir}/${plugin_dir}/"*.so "${appdir}/usr/plugins/${plugin_dir}/"
        fi
    done
else
    echo "Warning: Qt Wayland platform plugin was not found; AppImage will fall back to XCB." >&2
fi

if [ -n "${qt_translations_dir}" ] && [ -d "${qt_translations_dir}" ]; then
    mkdir -p "${appdir}/usr/translations"
    for language in en fr; do
        for catalog in qt qtbase; do
            translation="${qt_translations_dir}/${catalog}_${language}.qm"
            if [ -f "${translation}" ]; then
                cp "${translation}" "${appdir}/usr/translations/"
            fi
        done
    done
fi

if [ ! -f "${desktop_file}" ]; then
    echo "Missing desktop file: ${desktop_file}" >&2
    exit 1
fi

if [ ! -f "${icon_file}" ]; then
    echo "Missing icon file: ${icon_file}" >&2
    exit 1
fi

export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="${qmake}"
export VERSION="${project_version}"
export OUTPUT="${output_abs}"
export LINUXDEPLOY_OUTPUT_VERSION="${project_version}"
export LDAI_OUTPUT="${output_abs}"
export PATH="${tools_dir}:${PATH}"

"${linuxdeploy}" \
    --appdir "${appdir}" \
    --plugin qt \
    --executable "${appdir}/usr/bin/linscreen" \
    --desktop-file "${desktop_file}" \
    --icon-file "${icon_file}" \
    --output appimage

if [ ! -f "${OUTPUT}" ]; then
    generated_appimage="$(find "${build_dir}" -maxdepth 1 -name '*.AppImage' | head -n 1)"
    if [ -n "${generated_appimage}" ]; then
        mv "${generated_appimage}" "${OUTPUT}"
    fi
fi

if [ ! -f "${OUTPUT}" ]; then
    echo "AppImage build finished, but ${OUTPUT} was not created." >&2
    exit 1
fi

chmod +x "${OUTPUT}"
echo "AppImage written to ${OUTPUT}"
