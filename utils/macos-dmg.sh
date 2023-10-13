#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

QTLIBS=("Core" "Gui" "OpenGL" "PrintSupport" "Svg" "Widgets")

rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__

rm -rf build/dmg
mkdir build/dmg

cp -rL build/mod-app.app build/dmg/
cp utils/macos-readme.txt build/dmg/README.txt

pushd build/dmg/mod-app.app/Contents

rm -rf Frameworks/*/*.prl
rm -rf Frameworks/*/Headers
rm -rf Frameworks/*/Versions
rm -rf MacOS/data

for f in $(ls Frameworks/*/Qt* PlugIns/*/libq*.dylib); do
    for q in "${QTLIBS[@]}"; do
        install_name_tool -change "@rpath/Qt${q}.framework/Versions/5/Qt${q}" "@executable_path/../Frameworks/Qt${q}.framework/Qt${q}" "${f}"
    done
done

for f in $(ls MacOS/lib/libmod_utils.so MacOS/libjack*.dylib); do
    install_name_tool -change "${HOME}/PawPawBuilds/targets/macos-universal-10.15/lib/libjack.0.1.0.dylib" "@executable_path/libjack.0.dylib" "${f}"
    install_name_tool -change "${HOME}/PawPawBuilds/targets/macos-universal-10.15/lib/libjackserver.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
done

for f in $(ls MacOS/jackd MacOS/jack/*.so); do
    install_name_tool -change "${HOME}/PawPawBuilds/targets/macos-universal-10.15/lib/libjack.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
    install_name_tool -change "${HOME}/PawPawBuilds/targets/macos-universal-10.15/lib/libjackserver.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
done

popd

hdiutil create "mod-app-$(make version)-macOS.dmg" -srcfolder build/dmg -volname "MOD App" -fs HFS+ -ov
rm -rf build/dmg
