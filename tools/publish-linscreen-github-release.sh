#!/usr/bin/env sh
set -eu

repo="${1:-aurelson101/linscreen}"
project_version="$(sed -n 's/^set(LINSCREEN_VERSION[[:space:]]*\([0-9][^)]*\)).*/\1/p' CMakeLists.txt | head -n 1)"
tag="${2:-v${project_version}}"
remote_name="${3:-linscreen}"
build_dir="${BUILD_DIR:-build-linscreen-release}"
appimage_build_dir="${APPIMAGE_BUILD_DIR:-${build_dir}-appimage}"
deb="${build_dir}/linscreen_${project_version}_amd64.deb"
tarball="${build_dir}/linscreen-${project_version}-linux.tar.gz"
appimage="${build_dir}/LinScreen-${project_version}-x86_64.AppImage"
notes="docs/releases/${tag}.md"

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

if [ ! -f "${deb}" ]; then
    tools/package-linscreen-deb.sh "${build_dir}"
fi

if [ ! -f "${appimage}" ]; then
    tools/package-linscreen-appimage.sh "${appimage_build_dir}"
    cp "${appimage_build_dir}/LinScreen-${project_version}-x86_64.AppImage" \
        "${appimage}"
fi

for artifact in "${deb}" "${tarball}" "${appimage}" "${notes}"; do
    if [ ! -f "${artifact}" ]; then
        echo "Missing release file: ${artifact}" >&2
        exit 1
    fi
done

if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Refusing to publish from a dirty worktree." >&2
    exit 1
fi

(
    cd "${build_dir}"
    sha256sum \
        "$(basename "${deb}")" \
        "$(basename "${tarball}")" \
        "$(basename "${appimage}")" > SHA256SUMS
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

head_commit="$(git rev-parse HEAD)"
if git rev-parse -q --verify "refs/tags/${tag}" >/dev/null; then
    tag_commit="$(git rev-list -n 1 "${tag}")"
    if [ "${tag_commit}" != "${head_commit}" ]; then
        echo "Refusing to move existing local tag ${tag}." >&2
        exit 1
    fi
else
    git tag "${tag}"
fi

remote_tag_commit="$(git ls-remote "${remote_name}" "refs/tags/${tag}^{}" | awk 'NR == 1 { print $1 }')"
if [ -z "${remote_tag_commit}" ]; then
    remote_tag_commit="$(git ls-remote "${remote_name}" "refs/tags/${tag}" | awk 'NR == 1 { print $1 }')"
fi
if [ -n "${remote_tag_commit}" ] && [ "${remote_tag_commit}" != "${head_commit}" ]; then
    echo "Refusing to move existing remote tag ${tag}." >&2
    exit 1
fi
git push "${remote_name}" "${tag}"

if gh release view "${tag}" --repo "${repo}" >/dev/null 2>&1; then
    gh release upload "${tag}" \
        "${deb}" \
        "${tarball}" \
        "${appimage}" \
        "${build_dir}/SHA256SUMS" \
        --repo "${repo}" \
        --clobber
else
    gh release create "${tag}" \
        "${deb}" \
        "${tarball}" \
        "${appimage}" \
        "${build_dir}/SHA256SUMS" \
        --repo "${repo}" \
        --title "LinScreen ${tag#v}" \
        --notes-file "${notes}"
fi

echo "Published https://github.com/${repo}/releases/tag/${tag}"
