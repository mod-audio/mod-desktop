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

# ---------------------------------------------------------------------------------------------------------------------
# run command

JACKD="./build/jackd"

if [ "${target}" = "win64" ]; then
JACKD+=".exe"
EXE_WRAPPER="wine"
fi

JACKD+=" -C utils/jack-session.conf -R -S"

if [ "${target}" = "win64" ]; then
JACKD+=" -X coremidi -d coreaudio"
elif [ "${target}" = "win64" ]; then
JACKD+=" -X winmme -d portaudio -d 'ASIO::WineASIO Driver'"
else
JACKD+=" -d dummy"
fi

JACKD+=" -r 48000 -p 128"

exec ${EXE_WRAPPER} ${JACKD}

# ---------------------------------------------------------------------------------------------------------------------
