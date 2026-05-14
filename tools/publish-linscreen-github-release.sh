#!/usr/bin/env sh
set -eu

repo="${1:-aurelson101/linscreen}"
project_version="$(sed -n 's/^set(LINSCREEN_VERSION[[:space:]]*\([0-9][^)]*\)).*/\1/p' CMakeLists.txt | head -n 1)"
tag="${2:-v${project_version}}"
remote_name="${3:-linscreen}"
build_dir="${BUILD_DIR:-build-linscreen-release}"

if ! command -v gh >/dev/null 2>&1; then
    echo "gh is required. Install GitHub CLI and run: gh auth login" >&2
    exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "GitHub CLI is not authenticated. Run: gh auth login" >&2
    exit 1
fi

if [ -z "${project_version}" ]; then
    echo "Could not read LINSCREEN_VERSION from CMakeLists.txt" >&2
    exit 1
fi

if [ ! -f "${build_dir}/linscreen_${project_version}_amd64.deb" ]; then
    tools/package-linscreen-deb.sh "${build_dir}"
fi

(
    cd "${build_dir}"
    sha256sum linscreen_*.deb linscreen-*-linux.tar.gz > SHA256SUMS
)

if ! gh repo view "${repo}" >/dev/null 2>&1; then
    gh repo create "${repo}" \
        --public \
        --description "Wayland-focused screenshot and annotation tool."
fi

repo_url="https://github.com/${repo}.git"
if git remote get-url "${remote_name}" >/dev/null 2>&1; then
    git remote set-url "${remote_name}" "${repo_url}"
else
    git remote add "${remote_name}" "${repo_url}"
fi

git push -u "${remote_name}" HEAD:main
git tag -f "${tag}"
git push -f "${remote_name}" "${tag}"

if gh release view "${tag}" --repo "${repo}" >/dev/null 2>&1; then
    gh release upload "${tag}" \
        "${build_dir}"/linscreen_*.deb \
        "${build_dir}"/linscreen-*-linux.tar.gz \
        "${build_dir}/SHA256SUMS" \
        --repo "${repo}" \
        --clobber
else
    gh release create "${tag}" \
        "${build_dir}"/linscreen_*.deb \
        "${build_dir}"/linscreen-*-linux.tar.gz \
        "${build_dir}/SHA256SUMS" \
        --repo "${repo}" \
        --title "LinScreen ${tag#v}" \
        --notes-file "docs/releases/${tag}.md"
fi

echo "Published https://github.com/${repo}/releases/tag/${tag}"
