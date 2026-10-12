#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit 1
fi

if ! find -L build/plugins -mindepth 1 -maxdepth 1 -type d -name '*.lv2' -print -quit 2>/dev/null | grep -q .; then
    echo "Refusing to package: build/plugins contains no LV2 bundles" >&2
    echo "Run 'make PAWPAW_TARGET=win64 restage-plugins' first" >&2
    exit 1
fi

# the installer must package the html of THIS source set: refresh build/html first
# (before 0.0.14.30 the installer packed a stale build/html left from an earlier build;
# only win64-zip.sh refreshed it)
./utils/pack-html.sh

# setup innosetup
dlfile="${PWD}/utils/innosetup-6.0.5.exe"
innodir="${PWD}/build/innosetup-6.0.5"
iscc="${innodir}/drive_c/InnoSetup/ISCC.exe"

# Download the pinned installer and reject an HTML download page left by a
# redirect or upstream error. Inno Setup's generic download endpoint no longer
# serves the requested executable directly.
if ! file "${dlfile}" 2>/dev/null | grep -q 'PE32'; then
    rm -f "${dlfile}"
    curl --fail --location --retry 3 --retry-delay 2 \
        https://github.com/jrsoftware/issrc/releases/download/is-6_0_5/innosetup-6.0.5.exe \
        -o "${dlfile}"
    file "${dlfile}" | grep -q 'PE32'
fi

# initialize wine
if [ ! -d "${innodir}"/drive_c ]; then
    env WINEPREFIX="${innodir}" wineboot -u
fi

# install innosetup in custom wineprefix
if [ ! -f "${innodir}"/drive_c/InnoSetup/ISCC.exe ]; then
    env WINEPREFIX="${innodir}" wine "${dlfile}" /allusers /dir=C:\\InnoSetup /nocancel /norestart /verysilent
fi

# generate pedalboards and plugins
echo -n "" > utils/win64/win64-plugins.iss
IFS='
'
for f in $(find -L build/plugins/ -type f); do
    d=$(dirname $(echo ${f} | sed "s|build/plugins/||"))
    echo "Source: \"..\\..\\$(echo ${f} | tr '/' '\\')\"; DestDir: \"{app}\\plugins\\$(echo ${d} | tr '/' '\\')\"; Components: plugins; Flags: ignoreversion;" >> utils/win64/win64-plugins.iss
done
for f in $(find -L pedalboards/ -type f); do
    d=$(dirname $(echo ${f} | sed "s|pedalboards/||"))
    echo "Source: \"..\\..\\$(echo ${f} | tr '/' '\\')\"; DestDir: \"{app}\\pedalboards\\$(echo ${d} | tr '/' '\\')\"; Components: pedalboards; Flags: ignoreversion;" >> utils/win64/win64-pedalboards.iss
done

# generate version
echo "#define VERSION \"$(cat VERSION)\"" > utils/win64/win64-version.iss

# create the installer file
pushd "utils/win64"
env WINEPREFIX="${innodir}" wine "${iscc}" "win64-installer.iss"
popd
