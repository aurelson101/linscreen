#!/usr/bin/env sh
set -eu

[ "$#" -le 1 ] || { echo "Usage: $0 [build-linscreen-flatpak[-suffix]]" >&2; exit 1; }
project_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)"
cd "$project_root"
work_dir="$(realpath -m "${1:-build-linscreen-flatpak}")"
case "$work_dir" in
    "$project_root/build-linscreen-flatpak"|"$project_root"/build-linscreen-flatpak-*) ;;
    *) echo "Build directory must be a build-linscreen-flatpak directory inside the project." >&2; exit 1 ;;
esac
for tool in git tar jq flatpak mktemp sha256sum; do
    command -v "$tool" >/dev/null 2>&1 || { echo "$tool is required." >&2; exit 1; }
done
if command -v flatpak-builder >/dev/null 2>&1; then
    run_builder() { flatpak-builder "$@"; }
elif flatpak info --user org.flatpak.Builder >/dev/null 2>&1; then
    run_builder() { flatpak run --command=flatpak-builder org.flatpak.Builder "$@"; }
else
    echo "Install flatpak-builder or the user Flatpak org.flatpak.Builder first." >&2
    exit 1
fi
if ! git diff --quiet HEAD || [ -n "$(git ls-files --others --exclude-standard)" ]; then
    echo "Commit changes before building a distributable Flatpak." >&2
    exit 1
fi
marker="$work_dir/.linscreen-flatpak-workspace"
if [ -d "$work_dir" ] && [ ! -f "$marker" ] &&
   [ -n "$(find "$work_dir" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
    echo "Refusing to reuse a non-empty, unmanaged build directory." >&2
    exit 1
fi
mkdir -p "$work_dir"
if [ -f "$marker" ] && [ "$(sed -n '1p' "$marker")" != "$project_root" ]; then
    echo "Build directory belongs to another checkout." >&2
    exit 1
fi
printf '%s\n' "$project_root" > "$marker"
source_dir="$(mktemp -d "$work_dir/source.XXXXXX")"
git archive HEAD | tar -x -C "$source_dir"
revision="$(git rev-parse --short HEAD)"
version="$(sed -n 's/^set(LINSCREEN_VERSION[[:space:]]*\([0-9][^)]*\)).*/\1/p' CMakeLists.txt)"
[ -n "$version" ] || { echo "Cannot determine version." >&2; exit 1; }
manifest_dir="$source_dir/packaging/flatpak"
run_builder --show-manifest "$manifest_dir/org.linscreen.LinScreen.yml" |
    jq --arg revision "$revision" '."build-options".env.GIT_HASH = $revision' > "$manifest_dir/resolved.json"
run_builder --user --force-clean --keep-build-dirs --jobs=2 \
    --state-dir="$work_dir/state" --repo="$work_dir/repo" \
    "$work_dir/appdir" "$manifest_dir/resolved.json"
bundle="LinScreen-${version}-x86_64.flatpak"
flatpak build-bundle --arch=x86_64 \
    --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    "$work_dir/repo" "$work_dir/$bundle" org.linscreen.LinScreen stable
(cd "$work_dir" && sha256sum "$bundle" > SHA256SUMS-flatpak)
printf 'Flatpak bundle: %s/%s\n' "$work_dir" "$bundle"
