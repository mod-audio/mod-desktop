#!/bin/bash

set -euo pipefail

if [ ! -d build ]; then
  echo "Please run this script from the root folder" >&2
  exit 1
fi

if ! find -L build/plugins -mindepth 1 -maxdepth 1 -type d -name '*.lv2' -print -quit 2>/dev/null | grep -q .; then
  echo "Refusing to package: build/plugins contains no LV2 bundles" >&2
  echo "Run 'make PAWPAW_TARGET=win64 restage-plugins' first" >&2
  exit 1
fi

rm -rf build/innosetup-*
rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__
./utils/pack-html.sh

VERSION="$(cat VERSION)"
PACKAGE="mod-desktop-${VERSION}-win64"
STAGING=$(mktemp -d "./.${PACKAGE}.stage.XXXXXX")
trap 'rm -rf "${STAGING}"' EXIT

cp -aL build "${STAGING}/${PACKAGE}"
(
  cd "${STAGING}"
  zip -r -9 "${PACKAGE}.zip" "${PACKAGE}"
)
mv "${STAGING}/${PACKAGE}.zip" "${PACKAGE}.zip"
