#!/bin/bash

# set -e

cd $(dirname "${0}")/../..

# ---------------------------------------------------------------------------------------------------------------------
# check target

target="${1}"
plugin="${2}"

if [ -z "${target}" ] || [ -z "${plugin}" ]; then
    echo "usage: ${0} <target> <plugin>"
    exit 1
fi

if [ ! -e src/mod-plugin-builder ]; then
    echo "missing src/mod-plugin-builder"
    exit 1
fi

# ---------------------------------------------------------------------------------------------------------------------
# import env

export PAWPAW_FAST_MATH=1
export PAWPAW_MODAUDIO=1
export PAWPAW_QUIET=1
export PAWPAW_SKIP_LTO=1
source src/PawPaw/local.env "${target}"

# ---------------------------------------------------------------------------------------------------------------------

export CFLAGS+="-D__MOD_DEVICES__ -D_MOD_DESKTOP"
export CXXFLAGS+="-D__MOD_DEVICES__ -D_MOD_DESKTOP -DJUCE_AUDIOPROCESSOR_NO_GUI=1"

export CMAKE
export PAWPAW_BUILDDIR
export PAWPAW_DOWNLOADDIR
export PAWPAW_PREFIX
export TARGET_CC
export TOOLCHAIN_PREFIX

if [ "${MACOS}" -eq 1 ]; then
    export PATH="${PAWPAW_PREFIX}-host/bin:${PATH}"
fi

make -f utils/plugin-builder/plugin-builder.mk pkgname="${plugin}" ${MAKE_ARGS} PREFIX=/usr NOOPT=true WITH_LTO=false VERBOSE=1

# ---------------------------------------------------------------------------------------------------------------------
