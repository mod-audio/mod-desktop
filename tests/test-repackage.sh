#!/bin/bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
FIXTURE=$(mktemp -d /tmp/mod-desktop-repackage-test.XXXXXX)
trap 'rm -rf "$FIXTURE"' EXIT

make_fixture() {
    local dir=$1
    mkdir -p "$dir/utils/linux" "$dir/utils/win64" "$dir/build/plugins/example.lv2" "$dir/build-plugin"
    cp "$ROOT/utils/linux/linux-tar.sh" "$dir/utils/linux/"
    cp "$ROOT/utils/win64/win64-zip.sh" "$dir/utils/win64/"
    printf '#!/bin/sh\nexit 0\n' > "$dir/utils/pack-html.sh"
    chmod +x "$dir/utils/pack-html.sh" "$dir/utils/linux/linux-tar.sh" "$dir/utils/win64/win64-zip.sh"
    printf '0.0.13\n' > "$dir/VERSION"
    printf 'plugin\n' > "$dir/build/plugins/example.lv2/manifest.ttl"
    printf 'runtime\n' > "$dir/build/mod-ui"
    printf 'variant\n' > "$dir/build-plugin/mod-desktop.so"
    printf 'launcher\n' > "$dir/utils/linux/mod-desktop.run"
    printf 'desktop\n' > "$dir/utils/linux/mod-desktop.desktop"
}

linux="$FIXTURE/linux"
make_fixture "$linux"
(
    cd "$linux"
    ./utils/linux/linux-tar.sh
    test -f build/mod-ui
    test -f build/plugins/example.lv2/manifest.ttl
    tar -tJf mod-desktop-0.0.13-linux-"$(uname -m)".tar.xz | grep -q '/mod-desktop/plugins/example.lv2/manifest.ttl$'
)

windows="$FIXTURE/windows"
make_fixture "$windows"
(
    cd "$windows"
    ./utils/win64/win64-zip.sh
    test -f build/mod-ui
    test -f build/plugins/example.lv2/manifest.ttl
    unzip -Z1 mod-desktop-0.0.13-win64.zip | grep -q '/plugins/example.lv2/manifest.ttl$'
)

restage="$FIXTURE/restage"
mkdir -p "$restage/cache/a.lv2" "$restage/cache/b.lv2" "$restage/build/plugins/obsolete.lv2"
printf a > "$restage/cache/a.lv2/manifest.ttl"
printf b > "$restage/cache/b.lv2/manifest.ttl"
"$ROOT/utils/restage-plugins.sh" "$restage/build/plugins" "$restage/cache" a.lv2 b.lv2
test -L "$restage/build/plugins/a.lv2"
test -L "$restage/build/plugins/b.lv2"
test ! -e "$restage/build/plugins/obsolete.lv2"

if "$ROOT/utils/restage-plugins.sh" "$restage/build/plugins" "$restage/cache" missing.lv2 2>/dev/null; then
    echo 'restage accepted a missing plugin bundle' >&2
    exit 1
fi
test -L "$restage/build/plugins/a.lv2"
test -L "$restage/build/plugins/b.lv2"

grep -Fq 'build\html\img\desktop-app\*.jpg' "$ROOT/utils/win64/win64-installer.iss"
grep -Fq 'build\html\include\desktop-app\*.html' "$ROOT/utils/win64/win64-installer.iss"
grep -Fq 'build\html\img\favicon\*.json' "$ROOT/utils/win64/win64-installer.iss"

echo "repackage regression tests passed"
