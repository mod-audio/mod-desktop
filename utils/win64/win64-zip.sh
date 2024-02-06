#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

rm -rf build/innosetup-*
rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__
./utils/pack-html.sh

VERSION="$(cat VERSION)"
mv build mod-desktop-${VERSION}-win64
zip -r -9 mod-desktop-${VERSION}-win64.zip mod-desktop-${VERSION}-win64
