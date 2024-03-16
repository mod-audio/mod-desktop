#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__
./utils/pack-html.sh

[ -n "${PAWPAW_DEBUG}" ] && [ "${PAWPAW_DEBUG}" -eq 1 ] && SUFFIX="-debug"

ARCH=$(uname -m)
VERSION="$(cat VERSION)"
mkdir -p mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}/plugin-variants
mv build mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}/mod-desktop
cp utils/linux/mod-desktop.* mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}/
cp -r build-plugin/*.* mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}/plugin-variants/
tar chJf mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}.tar.xz mod-desktop-${VERSION}-linux-${ARCH}${SUFFIX}
