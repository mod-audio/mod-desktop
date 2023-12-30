#!/bin/bash

cd "$(dirname $0)/../../build"

export LANG=en_US.UTF-8
export PAWPAW_SKIP_LTO=1
export PAWPAW_QUIET=1

function convert_path() {
    if [ -e jackd.exe ]; then
        echo "Z:\\$(echo ${@} | tr '/' '\\')"
    else
        echo ${@}
    fi
}

if [ -e mod-ui.exe ]; then
    source ../src/PawPaw/local.env win64
    DOCS_DIR=$(xdg-user-dir DOCUMENTS)
    LV2_PATH="$(convert_path $(pwd)/plugins)"
    OS_SEP="\\"
elif [ -e mod-app.app ]; then
    if [ "$(uname -m)" = "x86_64" ] && [ x"${1}" != x"macos-universal-10.15" ]; then
        source ../src/PawPaw/local.env macos-10.15
    else
        source ../src/PawPaw/local.env macos-universal-10.15
    fi
    DOCS_DIR=~/Documents
    LV2_PATH="$(pwd)/mod-app.app/Contents/PlugIns/LV2"
    OS_SEP='/'
else
    source ../src/PawPaw/local.env linux
    DOCS_DIR=$(xdg-user-dir DOCUMENTS)
    LV2_PATH="$(pwd)/plugins"
    OS_SEP='/'
fi

export LV2_PATH

export CARLA_BRIDGE_DUMMY=1
export CARLA_BRIDGE_TESTING=1
export MOD_KEYS_PATH="$(convert_path ${DOCS_DIR}/MOD App/keys/)"
export MOD_USER_FILES_DIR="$(convert_path ${DOCS_DIR}/MOD App/user-files)"
export WINEDEBUG=-all

set -e

mkdir -p "${DOCS_DIR}/MOD App/keys"
mkdir -p "${DOCS_DIR}/MOD App/user-files/Audio Recordings"

if [ -z "${EXE_WRAPPER}" ] && [ -n "${VALGRIND}" ] && [ "${VALGRIND}" -eq 1 ]; then
    EXE_WRAPPER="valgrind --leak-check=full --track-origins=yes --keep-debuginfo=yes --suppressions=../src/mod-plugin-builder/plugins-dep/valgrind-libdl.supp"
fi

PLUGINS=($(${EXE_WRAPPER} "${PAWPAW_PREFIX}/lib/carla/carla-discovery-native${APP_EXT}" lv2 ":all" 2>/dev/null | tr -dC '[:print:]\n' | awk 'sub("carla-discovery::label::","")'))

for p in ${PLUGINS[@]}; do
    uri=$(echo ${p} | cut -d "${OS_SEP}" -f 2-)
    echo "Testing ${uri}..."
    ${EXE_WRAPPER} "${PAWPAW_PREFIX}/lib/carla/carla-bridge-native${APP_EXT}" lv2 "" "${uri}" 1>/dev/null
done
