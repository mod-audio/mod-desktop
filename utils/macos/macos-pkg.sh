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
./utils/pack-html.sh

# create pkg dir for placing patched app bundle inside
mkdir build/pkg build/pkg/app
gcp -rL "build/mod-desktop.app" "build/pkg/app/MOD Desktop.app"

# create pkg dir for plugins
mkdir build/pkg/au build/pkg/clap build/pkg/lv2 build/pkg/vst2 build/pkg/vst3
gcp -rL build-plugin/*.component build/pkg/au/
gcp -rL build-plugin/*.clap build/pkg/clap/
gcp -rL build-plugin/*.lv2 build/pkg/lv2/
gcp -rL build-plugin/*.vst build/pkg/vst2/
gcp -rL build-plugin/*.vst3 build/pkg/vst3/

# patch rpath for Qt libs and jack tools
pushd "build/pkg/app/MOD Desktop.app/Contents"

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

# sign app bundle
if [ -n "${CODESIGN_APP_IDENTITY}" ]; then
    security create-keychain -p dummypassword build.keychain
    security unlock-keychain -p dummypassword build.keychain
    security set-keychain-settings -lut 21600 build.keychain

    echo "${CODESIGN_APP_P12_CONTENTS}" | base64 -d -o codesign.p12
    security import codesign.p12 -f pkcs12 -P "${CODESIGN_APP_P12_PASSWORD}" -k build.keychain -T /usr/bin/codesign -T /usr/bin/security
    rm codesign.p12

    echo "${CODESIGN_PKG_P12_CONTENTS}" | base64 -d -o codesign.p12
    security import codesign.p12 -f pkcs12 -P "${CODESIGN_PKG_P12_PASSWORD}" -k build.keychain -T /usr/bin/pkgbuild -T /usr/bin/productbuild -T /usr/bin/security
    rm codesign.p12

    security set-key-partition-list -S apple-tool:,apple: -k dummypassword build.keychain
    security list-keychains -d user -s build.keychain login.keychain

    pushd "build/pkg/app/MOD Desktop.app/Contents/LV2"

    for f in $(find . -name "*.dylib"); do
        codesign -s "${CODESIGN_APP_IDENTITY}" \
            --force \
            --verbose \
            --timestamp \
            --option runtime \
            --entitlements "../../../../../../utils/macos/entitlements.plist" \
            "${f}"
    done

    popd

    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --deep \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        "build/pkg/app/MOD Desktop.app"

    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --deep \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        build/pkg/clap/*.clap

    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        build/pkg/lv2/*.lv2/*.dylib

    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --deep \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        build/pkg/vst2/*.vst

    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --deep \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        build/pkg/vst3/*.vst3

    PKG_SIGN_ARGS=(--sign "${CODESIGN_PKG_IDENTITY}")
fi

# create base app pkg
pkgbuild \
  --identifier "audio.mod.desktop-app" \
  --component-plist "utils/macos/build.plist" \
  --install-location "/Applications/" \
  --root "${PWD}/build/pkg/app/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop.pkg

# create plugins pkgs
pkgbuild \
  --identifier "audio.mod.desktop-components" \
  --install-location "/Library/Audio/Plug-Ins/Components/" \
  --root "${PWD}/build/pkg/au/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop-components.pkg

pkgbuild \
  --identifier "audio.mod.desktop-clapbundles" \
  --install-location "/Library/Audio/Plug-Ins/CLAP/" \
  --root "${PWD}/build/pkg/clap/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop-clapbundles.pkg

pkgbuild \
  --identifier "audio.mod.desktop-lv2bundles" \
  --install-location "/Library/Audio/Plug-Ins/LV2/" \
  --root "${PWD}/build/pkg/lv2/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop-lv2bundles.pkg

pkgbuild \
  --identifier "audio.mod.desktop-vst2bundles" \
  --install-location "/Library/Audio/Plug-Ins/VST/" \
  --root "${PWD}/build/pkg/vst2/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop-vst2bundles.pkg

pkgbuild \
  --identifier "audio.mod.desktop-vst3bundles" \
  --install-location "/Library/Audio/Plug-Ins/VST3/" \
  --root "${PWD}/build/pkg/vst3/" \
  "${PKG_SIGN_ARGS[@]}" \
  build/mod-desktop-vst3bundles.pkg

# create final pkg
sed -e "s|@builddir@|${PWD}/build|" \
    utils/macos/package.xml.in > build/package.xml

productbuild \
  --distribution build/package.xml \
  --identifier "audio.mod.desktop-app" \
  --package-path "${PWD}/build" \
  --version 0 \
  "${PKG_SIGN_ARGS[@]}" \
  mod-desktop-$(cat VERSION)-macOS.pkg

# cleanup
rm -rf build/pkg
[ -n "${CODESIGN_APP_IDENTITY}" ] && security delete-keychain build.keychain

exit 0
