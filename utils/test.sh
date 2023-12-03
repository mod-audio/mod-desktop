#!/bin/bash

set -e

cd $(dirname "${0}")/..

# ---------------------------------------------------------------------------------------------------------------------
# check target

target="${1}"

if [ -z "${target}" ]; then
    echo "usage: ${0} <target>"
    exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------
# set up env

export LV2_PATH=$(pwd)/build/plugins
# export MOD_LOG=1

export JACK_NO_START_SERVER=1

# ---------------------------------------------------------------------------------------------------------------------
# run command

if [ "${target}" = "macos-universal-10.15" ]; then
    export JACK_DRIVER_DIR="$(pwd)/build/mod-app.app/Contents/MacOS/jack"
    JACKD="./build/mod-app.app/Contents/MacOS/jackd"
else
    export JACK_DRIVER_DIR="$(pwd)/build/jack"
    JACKD="./build/jackd"
fi

if [ "${target}" = "win64" ]; then
    JACKD+=".exe"
    EXE_WRAPPER="wine"
fi
# EXE_WRAPPER="lldb -- "

JACKD+=" -R -S -C utils/jack-session.conf"

if [ "${target}" = "macos-universal-10.15" ]; then
    # JACKD+=" -X coremidi"
    JACKD+=" -d coreaudio"
    JACKD+=" -P 'Built-in'"
elif [ "${target}" = "win64" ]; then
    JACKD+=" -X winmme"
    JACKD+=" -d portaudio"
    JACKD+=" -d 'ASIO::WineASIO Driver'"
else
    # -X alsarawmidi
    JACKD+=" -d portaudio"
    JACKD+=" -d 'ALSA::pulse'"
fi

JACKD+=" -r 48000"
JACKD+=" -p 256"
# JACKD+=" -p 8192"
# JACKD+=" -h"

exec ${EXE_WRAPPER} ${JACKD}

# ---------------------------------------------------------------------------------------------------------------------
