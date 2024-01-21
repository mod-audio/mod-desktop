#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

if [ "$(uname -m)" = "x86_64" ] && [ x"${1}" != x"macos-universal-10.15" ]; then
    PAWPAW_PREFIX="${HOME}/PawPawBuilds/targets/macos-10.15"
else
    PAWPAW_PREFIX="${HOME}/PawPawBuilds/targets/macos-universal-10.15"
fi

rm -rf build/pkg build/*.pkg
rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__

# create pkg dir for placing patched app bundle inside
mkdir build/pkg
gcp -rL "build/mod-desktop-app.app" "build/pkg/MOD Desktop App.app"

# patch rpath for Qt libs and jack tools
pushd "build/pkg/MOD Desktop App.app/Contents"

rm -rf Frameworks/*/*.prl
rm -rf Frameworks/*/Headers
rm -rf Frameworks/*/Versions
rm -rf MacOS/data

QTLIBS=("Core" "Gui" "OpenGL" "PrintSupport" "Svg" "Widgets")

for f in $(ls Frameworks/*/Qt* PlugIns/*/libq*.dylib); do
    for q in "${QTLIBS[@]}"; do
        install_name_tool -change "@rpath/Qt${q}.framework/Versions/5/Qt${q}" "@executable_path/../Frameworks/Qt${q}.framework/Qt${q}" "${f}"
    done
done

for f in $(ls MacOS/lib/libmod_utils.so MacOS/libjack*.dylib); do
    install_name_tool -change "${PAWPAW_PREFIX}/lib/libjack.0.1.0.dylib" "@executable_path/libjack.0.dylib" "${f}"
    install_name_tool -change "${PAWPAW_PREFIX}/lib/libjackserver.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
done

for f in $(ls MacOS/jackd MacOS/jack/*.so); do
    install_name_tool -change "${PAWPAW_PREFIX}/lib/libjack.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
    install_name_tool -change "${PAWPAW_PREFIX}/lib/libjackserver.0.1.0.dylib" "@executable_path/libjackserver.0.dylib" "${f}"
done

popd

# create base app pkg
pkgbuild \
  --identifier "audio.mod.desktop-app" \
  --component-plist "utils/macos/build.plist" \
  --install-location "/Applications/" \
  --root "${PWD}/build/pkg/" \
  build/mod-desktop-app.pkg

# create final pkg
sed -e "s|@builddir@|${PWD}/build|" \
    utils/macos/package.xml.in > build/package.xml

productbuild \
  --distribution build/package.xml \
  --identifier "audio.mod.desktop-app" \
  --package-path "${PWD}/build" \
  --version 0 \
  mod-desktop-app-$(cat VERSION)-macOS.pkg

# cleanup
rm -rf build/pkg
