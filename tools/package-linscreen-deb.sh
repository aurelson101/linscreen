#!/usr/bin/env sh
set -eu

build_dir="${1:-build-linscreen-release}"

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake is required to build LinScreen." >&2
    exit 1
fi

cmake -S . -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DDISABLE_UPDATE_CHECKER=ON \
    -DENABLE_IMGUR=OFF

cmake --build "${build_dir}" -j"$(nproc)"
cmake --build "${build_dir}" --target package

echo "Packages written under ${build_dir}/"
