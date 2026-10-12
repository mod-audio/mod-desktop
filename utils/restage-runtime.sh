#!/bin/bash

set -euo pipefail

TARGET=${1:-}
ROOT=$(pwd)

case "${TARGET}" in
  linux-*)
    APP_EXT=""
    SO_EXT=".so"
    PAWPAW_PREFIX="${HOME}/PawPawBuilds/targets/${TARGET}"
    DESCRIPTOR="utils/linux/mod-hardware-descriptor.json"
    ;;
  win64)
    APP_EXT=".exe"
    SO_EXT=".dll"
    PAWPAW_PREFIX="${HOME}/PawPawBuilds/targets/win64"
    DESCRIPTOR="utils/win64/mod-hardware-descriptor.json"
    ;;
  macos-*)
    # The macOS app bundle is retained in place; only its build/plugins target
    # is reconstructed by restage-plugins.
    exit 0
    ;;
  *)
    echo "Unsupported package target: ${TARGET}" >&2
    exit 2
    ;;
esac

STAGING=$(mktemp -d "${ROOT}/.build.stage.XXXXXX")
trap 'rm -rf "${STAGING}"' EXIT

link_output() {
    local source=$1
    local destination=$2
    if [ ! -e "${source}" ]; then
        echo "Missing retained package input: ${source}" >&2
        exit 1
    fi
    mkdir -p "$(dirname "${STAGING}/${destination}")"
    ln -s "${source}" "${STAGING}/${destination}"
}

# Recreate the cx_Freeze library directory entry-by-entry. The retained
# libmod_utils link was created inside a build container and points at /src;
# replace that one link with a target-worktree path while preserving all other
# frozen output without modifying the root/container-owned build-ui directory.
mkdir -p "${STAGING}/lib"
shopt -s nullglob
for source in "${ROOT}"/build-ui/lib/*; do
    name=$(basename "${source}")
    [ "${name}" = "libmod_utils${SO_EXT}" ] && continue
    link_output "${source}" "lib/${name}"
done
shopt -u nullglob
link_output "${ROOT}/src/mod-ui/utils/libmod_utils${SO_EXT}" "lib/libmod_utils${SO_EXT}"

link_output "${ROOT}/src/mod-ui/default.pedalboard" default.pedalboard
link_output "${ROOT}/src/mod-ui/html" html
link_output "${PAWPAW_PREFIX}/bin/jackd${APP_EXT}" "jackd${APP_EXT}"
link_output "${ROOT}/utils/jack/jack-session.conf" jack/jack-session.conf
link_output "${ROOT}/src/mod-host/mod-host.so" "jack/mod-host${SO_EXT}"
link_output "${ROOT}/build-midi-merger/mod-midi-broadcaster.so" "jack/mod-midi-broadcaster${SO_EXT}"
link_output "${ROOT}/build-midi-merger/mod-midi-merger.so" "jack/mod-midi-merger${SO_EXT}"
link_output "${ROOT}/src/systray/mod-desktop${APP_EXT}" "mod-desktop${APP_EXT}"
link_output "${ROOT}/build-pedalboard/mod-pedalboard${APP_EXT}" "mod-pedalboard${APP_EXT}"
link_output "${ROOT}/build-ui/mod-ui${APP_EXT}" "mod-ui${APP_EXT}"
link_output "${ROOT}/src/mod-ui/mod" mod
link_output "${ROOT}/src/mod-ui/modtools" modtools
link_output "${ROOT}/${DESCRIPTOR}" mod-hardware-descriptor.json
link_output "${ROOT}/pedalboards" pedalboards
link_output "${ROOT}/VERSION" VERSION

if [[ "${TARGET}" == linux-* ]]; then
    link_output "${PAWPAW_PREFIX}/lib/jack/alsa_midi.so" jack/alsa_midi.so
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_alsa.so" jack/jack_alsa.so
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_dummy.so" jack/jack_dummy.so
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_mod-desktop.so" jack/jack_mod-desktop.so
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_portaudio.so" jack/jack_portaudio.so
    link_output "${ROOT}/utils/jack/jack-session-alsamidi.conf" jack/jack-session-alsamidi.conf
    link_output "${PAWPAW_PREFIX}/lib/libjack.so.0" libjack.so.0
    link_output "${PAWPAW_PREFIX}/lib/libjackserver.so.0" libjackserver.so.0
else
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_dummy.dll" jack/jack_dummy.dll
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_mod-desktop.dll" jack/jack_mod-desktop.dll
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_portaudio.dll" jack/jack_portaudio.dll
    link_output "${PAWPAW_PREFIX}/lib/jack/jack_winmme.dll" jack/jack_winmme.dll
    link_output "${PAWPAW_PREFIX}/lib/libjack64.dll" libjack64.dll
    link_output "${PAWPAW_PREFIX}/lib/libjackserver64.dll" libjackserver64.dll
    link_output "${PAWPAW_PREFIX}/bin/libpython3.8.dll" libpython3.8.dll
    link_output "${ROOT}/src/mod-desktop-asio/mod-desktop-asio.dll" mod-desktop-asio.dll
    for qt in Core Gui Svg Widgets; do
        link_output "${PAWPAW_PREFIX}/bin/Qt5${qt}.dll" "Qt5${qt}.dll"
    done
    link_output "${PAWPAW_PREFIX}/lib/qt5/plugins/generic/qtuiotouchplugin.dll" generic/qtuiotouchplugin.dll
    link_output "${PAWPAW_PREFIX}/lib/qt5/plugins/iconengines/qsvgicon.dll" iconengines/qsvgicon.dll
    link_output "${PAWPAW_PREFIX}/lib/qt5/plugins/imageformats/qsvg.dll" imageformats/qsvg.dll
    link_output "${PAWPAW_PREFIX}/lib/qt5/plugins/platforms/qwindows.dll" platforms/qwindows.dll
    link_output "${PAWPAW_PREFIX}/lib/qt5/plugins/styles/qwindowsvistastyle.dll" styles/qwindowsvistastyle.dll
fi

BACKUP="${ROOT}/.build.previous.$$"
if [ -e "${ROOT}/build" ] || [ -L "${ROOT}/build" ]; then
    mv "${ROOT}/build" "${BACKUP}"
fi
mv "${STAGING}" "${ROOT}/build"
if [ -e "${BACKUP}" ] || [ -L "${BACKUP}" ]; then
    rm -rf "${BACKUP}"
fi
trap - EXIT
