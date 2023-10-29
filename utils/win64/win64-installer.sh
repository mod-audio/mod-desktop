#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

# setup innosetup
dlfile="${PWD}/utils/innosetup-6.0.5.exe"
innodir="${PWD}/build/innosetup-6.0.5"
iscc="${innodir}/drive_c/InnoSetup/ISCC.exe"

# download it
if [ ! -f "${dlfile}" ]; then
    # FIXME proper dl version
    curl -L https://jrsoftware.org/download.php/is.exe -o "${dlfile}"
fi

# initialize wine
if [ ! -d "${innodir}"/drive_c ]; then
    env WINEPREFIX="${innodir}" wineboot -u
fi

# install innosetup in custom wineprefix
if [ ! -f "${innodir}"/drive_c/InnoSetup/ISCC.exe ]; then
    env WINEPREFIX="${innodir}" wine "${dlfile}" /allusers /dir=C:\\InnoSetup /nocancel /norestart /verysilent
fi

# generate plugins
echo -n "" > utils/win64/win64-plugins.iss
IFS='
'
for f in $(find -L build/plugins/ -type f); do
    d=$(dirname $(echo ${f} | sed "s|build/plugins/||"))
    echo "Source: \"..\\..\\$(echo ${f} | tr '/' '\\')\"; DestDir: \"{app}\\plugins\\$(echo ${d} | tr '/' '\\')\"; Components: plugins; Flags: ignoreversion;" >> utils/win64/win64-plugins.iss
done

# generate version
echo "#define VERSION \"$(cat VERSION)\"" > utils/win64/win64-version.iss

# create the installer file
pushd "utils/win64"
env WINEPREFIX="${innodir}" wine "${iscc}" "win64-installer.iss"
popd
