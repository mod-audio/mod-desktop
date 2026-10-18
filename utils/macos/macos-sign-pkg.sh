#!/bin/bash
# Sign an already built, unsigned MOD Desktop macOS installer.
#
#   ./utils/macos/macos-sign-pkg.sh <unsigned.pkg> <signed.pkg>
#
# The installer is unpacked, its contents are signed the same way
# macos-pkg.sh signs them, and each component package is rebuilt with the
# identifier, version and install location it already had. Nothing else in
# the payload changes.
#
# Needs the same CODESIGN_* variables as macos-pkg.sh. If NOTARY_KEY_P8_CONTENTS
# (base64 of an App Store Connect API key), NOTARY_KEY_ID and NOTARY_ISSUER_ID
# are set too, the result is also notarized and stapled.

set -e

if [ $# -ne 2 ]; then
    echo "usage: $0 <unsigned.pkg> <signed.pkg>" >&2
    exit 1
fi

IN="$(cd "$(dirname "${1}")" && pwd)/$(basename "${1}")"
OUT="$(mkdir -p "$(dirname "${2}")" && cd "$(dirname "${2}")" && pwd)/$(basename "${2}")"
ROOT="$(cd "$(dirname "${0}")/../.." && pwd)"
ENTITLEMENTS="${ROOT}/utils/macos/entitlements.plist"

for v in CODESIGN_APP_IDENTITY CODESIGN_PKG_IDENTITY \
         CODESIGN_APP_P12_CONTENTS CODESIGN_PKG_P12_CONTENTS \
         CODESIGN_APP_P12_PASSWORD CODESIGN_PKG_P12_PASSWORD; do
    if [ -z "${!v}" ]; then
        echo "Missing ${v}" >&2
        exit 1
    fi
done

WORK="$(mktemp -d)"
KEYCHAIN="${WORK}/build.keychain"

cleanup() {
    security delete-keychain "${KEYCHAIN}" 2>/dev/null || true
    rm -rf "${WORK}"
}
trap cleanup EXIT

# temporary keychain with both certificates, as in macos-pkg.sh
security create-keychain -p dummypassword "${KEYCHAIN}"
security unlock-keychain -p dummypassword "${KEYCHAIN}"
security set-keychain-settings -lut 21600 "${KEYCHAIN}"

echo "${CODESIGN_APP_P12_CONTENTS}" | base64 -d -o "${WORK}/app.p12"
security import "${WORK}/app.p12" -f pkcs12 -P "${CODESIGN_APP_P12_PASSWORD}" -k "${KEYCHAIN}" -T /usr/bin/codesign -T /usr/bin/security
rm "${WORK}/app.p12"

echo "${CODESIGN_PKG_P12_CONTENTS}" | base64 -d -o "${WORK}/pkg.p12"
security import "${WORK}/pkg.p12" -f pkcs12 -P "${CODESIGN_PKG_P12_PASSWORD}" -k "${KEYCHAIN}" -T /usr/bin/pkgbuild -T /usr/bin/productbuild -T /usr/bin/security
rm "${WORK}/pkg.p12"

security set-key-partition-list -S apple-tool:,apple: -k dummypassword "${KEYCHAIN}" >/dev/null
security list-keychains -d user -s "${KEYCHAIN}" login.keychain

sign() {
    codesign -s "${CODESIGN_APP_IDENTITY}" \
        --keychain "${KEYCHAIN}" \
        --force \
        --verbose \
        --timestamp \
        --option runtime \
        --entitlements "${ENTITLEMENTS}" \
        "$@"
}

pkgutil --expand-full "${IN}" "${WORK}/x"
mkdir "${WORK}/out"

for comp in "${WORK}"/x/*.pkg; do
    name="$(basename "${comp}")"
    info="${comp}/PackageInfo"
    payload="${comp}/Payload"
    id="$(xmllint --xpath 'string(/pkg-info/@identifier)' "${info}")"
    version="$(xmllint --xpath 'string(/pkg-info/@version)' "${info}")"
    location="$(xmllint --xpath 'string(/pkg-info/@install-location)' "${info}")"
    extra=()

    echo "== ${name} (${id} -> ${location})"

    case "${name}" in
    mod-desktop.pkg)
        app="${payload}/MOD Desktop.app"
        find "${app}/Contents/LV2" -name "*.dylib" -print0 | while IFS= read -r -d '' f; do sign "${f}"; done
        sign --deep "${app}"
        extra=(--component-plist "${ROOT}/utils/macos/build.plist")
        ;;
    mod-desktop-lv2bundles.pkg)
        for f in "${payload}"/*.lv2/*.dylib; do sign "${f}"; done
        ;;
    *)
        # AU components, CLAP, VST2 and VST3 bundles
        for b in "${payload}"/*; do sign --deep "${b}"; done
        ;;
    esac

    pkgbuild \
        --identifier "${id}" \
        --version "${version}" \
        --install-location "${location}" \
        --root "${payload}" \
        "${extra[@]}" \
        --sign "${CODESIGN_PKG_IDENTITY}" \
        --keychain "${KEYCHAIN}" \
        "${WORK}/out/${name}"
done

productbuild \
    --distribution "${WORK}/x/Distribution" \
    --resources "${WORK}/x/Resources" \
    --package-path "${WORK}/out" \
    --sign "${CODESIGN_PKG_IDENTITY}" \
    --keychain "${KEYCHAIN}" \
    "${OUT}"

# checks: a failure here stops before anything is published
app="${WORK}/x/mod-desktop.pkg/Payload/MOD Desktop.app"
codesign --verify --deep --strict --verbose=2 "${app}"
for arch in arm64 x86_64; do
    for bin in mod-desktop jackd; do
        codesign -d --arch "${arch}" --entitlements - "${app}/Contents/MacOS/${bin}" 2>/dev/null \
            | grep -q "com.apple.security.device.audio-input" \
            || { echo "No audio-input entitlement on ${bin} (${arch})" >&2; exit 1; }
    done
done
pkgutil --check-signature "${OUT}"

if [ -n "${NOTARY_KEY_P8_CONTENTS}" ] && [ -n "${NOTARY_KEY_ID}" ] && [ -n "${NOTARY_ISSUER_ID}" ]; then
    echo "${NOTARY_KEY_P8_CONTENTS}" | base64 -d -o "${WORK}/notary.p8"
    xcrun notarytool submit "${OUT}" \
        --key "${WORK}/notary.p8" \
        --key-id "${NOTARY_KEY_ID}" \
        --issuer "${NOTARY_ISSUER_ID}" \
        --wait
    rm "${WORK}/notary.p8"
    xcrun stapler staple "${OUT}"
    spctl --assess --type install --verbose=2 "${OUT}"
else
    echo "Notarization skipped (NOTARY_* not set): signed, not notarized"
fi

exit 0
