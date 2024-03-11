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

rm -rf build/dmg build/*.dmg
rm -rf mod-ui/mod/__pycache__
rm -rf mod-ui/mod/communication/__pycache__
rm -rf mod-ui/modtools/__pycache__
./utils/pack-html.sh

# create dmg dir for placing patched app bundle inside
mkdir build/dmg
gcp -rL "build/mod-desktop.app" "build/dmg/MOD Desktop.app"

# patch rpath for Qt libs and jack tools
pushd "build/dmg/MOD Desktop.app/Contents"

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

    security set-key-partition-list -S apple-tool:,apple: -k dummypassword build.keychain
    security list-keychains -d user -s build.keychain login.keychain

    pushd "build/pkg/MOD Desktop.app/Contents/LV2"

    for f in $(find . -name "*.dylib"); do
        codesign -s "${CODESIGN_APP_IDENTITY}" \
            --force \
            --verbose \
            --timestamp \
            --option runtime \
            --entitlements "../../../../../utils/macos/entitlements.plist" \
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
        "build/pkg/MOD Desktop.app"
fi

# create dmg
hdiutil create "mod-desktop-$(cat VERSION)-macOS.dmg" -srcfolder build/dmg -volname "MOD Desktop" -fs HFS+ -ov

if [ -n "${CODESIGN_IDENTITY}" ]; then
    codesign -s "${MACOS_APP_DEV_ID}" \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "utils/macos/entitlements.plist" \
        "mod-desktop-$(cat VERSION)-macOS.dmg"
    security delete-keychain build.keychain
fi

# cleanup
rm -rf build/dmg
