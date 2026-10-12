#!/bin/bash

set -euo pipefail

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 DESTINATION SOURCE_ROOT BUNDLE..." >&2
    exit 2
fi

DESTINATION=$1
SOURCE_ROOT=$2
shift 2

for bundle in "$@"; do
    if [ ! -d "${SOURCE_ROOT}/${bundle}" ]; then
        echo "Missing cached plugin bundle: ${SOURCE_ROOT}/${bundle}" >&2
        exit 1
    fi
done

PARENT=$(dirname "${DESTINATION}")
NAME=$(basename "${DESTINATION}")
mkdir -p "${PARENT}"
STAGING=$(mktemp -d "${PARENT}/.${NAME}.stage.XXXXXX")
# mktemp creates 0700; the staging dir becomes the app's plugin dir (macOS Contents/LV2) and must be world-readable
chmod 755 "${STAGING}"
trap 'rm -rf "${STAGING}"' EXIT

for bundle in "$@"; do
    ln -s "${SOURCE_ROOT}/${bundle}" "${STAGING}/${bundle}"
done

BACKUP="${PARENT}/.${NAME}.previous.$$"
if [ -e "${DESTINATION}" ] || [ -L "${DESTINATION}" ]; then
    mv "${DESTINATION}" "${BACKUP}"
fi
mv "${STAGING}" "${DESTINATION}"
if [ -e "${BACKUP}" ] || [ -L "${BACKUP}" ]; then
    rm -rf "${BACKUP}"
fi
trap - EXIT
