#!/usr/bin/env sh
set -eu

repo_dir="${1:-flatpak-repo}"
build_dir="${2:-flatpak-build}"
manifest="packaging/flatpak/org.linscreen.LinScreen.yml"

if ! command -v flatpak-builder >/dev/null 2>&1; then
    echo "flatpak-builder is required to build the LinScreen Flatpak." >&2
    exit 1
fi

flatpak-builder --force-clean --repo="${repo_dir}" "${build_dir}" "${manifest}"
flatpak build-bundle "${repo_dir}" LinScreen.flatpak org.linscreen.LinScreen

echo "Flatpak bundle written to LinScreen.flatpak"
