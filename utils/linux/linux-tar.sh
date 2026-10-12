#!/bin/bash

set -euo pipefail

if [ ! -d build ]; then
  echo "Please run this script from the root folder" >&2
  exit 1
fi

if ! find -L build/plugins -mindepth 1 -maxdepth 1 -type d -name '*.lv2' -print -quit 2>/dev/null | grep -q .; then
  echo "Refusing to package: build/plugins contains no LV2 bundles" >&2
  echo "Run 'make restage-plugins' from the appropriate target environment first" >&2
  exit 1
fi

rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__
./utils/pack-html.sh

[ -n "${PAWPAW_DEBUG:-}" ] && [ "${PAWPAW_DEBUG}" -eq 1 ] && SUFFIX="-debug" || SUFFIX=""

ARCH=$(uname -m)
VERSION="$(cat VERSION)"
PACKAGE="mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}"
STAGING=$(mktemp -d "./.${PACKAGE}.stage.XXXXXX")
trap 'rm -rf "${STAGING}"' EXIT

mkdir -p "${STAGING}/${PACKAGE}/plugin-variants"
cp -aL build "${STAGING}/${PACKAGE}/mod-desktop"
cp utils/linux/mod-desktop.* "${STAGING}/${PACKAGE}/"
cp -aL build-plugin/*.* "${STAGING}/${PACKAGE}/plugin-variants/"

tar chJf "${STAGING}/${PACKAGE}.tar.xz" -C "${STAGING}" "${PACKAGE}"
mv "${STAGING}/${PACKAGE}.tar.xz" "${PACKAGE}.tar.xz"
