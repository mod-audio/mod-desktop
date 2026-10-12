#!/bin/bash
# Apply the MOD Desktop patches to the pinned submodules (src/PawPaw, src/mod-host, src/mod-plugin-builder).
# Run once after `git submodule update --init`, before `make`. Safe to re-run: already-applied patches are skipped.
# PawPaw-macos.patch (macOS toolchain/bootstrap fixes) is applied only on macOS, or when MOD_DESKTOP_PATCH_MACOS=1.
set -e
cd "$(dirname "$0")/.."

patches="PawPaw:PawPaw mod-host:mod-host mod-plugin-builder:mod-plugin-builder"
if [ "$(uname -s)" = "Darwin" ] || [ "${MOD_DESKTOP_PATCH_MACOS}" = "1" ]; then
    patches="${patches} PawPaw:PawPaw-macos"
fi

for entry in ${patches}; do
    dir="src/${entry%%:*}"
    name="${entry##*:}"
    patch="${PWD}/patches/${name}.patch"
    if git -C "${dir}" apply --check --reverse "${patch}" 2>/dev/null; then
        echo "${name}: already applied"
    elif git -C "${dir}" apply --check "${patch}"; then
        git -C "${dir}" apply "${patch}"
        echo "${name}: applied"
    else
        echo "${name}: does not apply to $(git -C "${dir}" rev-parse --short HEAD)" >&2
        exit 1
    fi
done
