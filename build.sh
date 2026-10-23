#!/usr/bin/env bash
# Build the HomeKey carrier TPK. Requires zig (brew install zig).
set -euo pipefail
cd "$(dirname "$0")"

rm -rf build && mkdir -p build/{bin,shared/res}

# Static armv7 stub (never executed; satisfies the manifest's exec reference).
zig cc -target arm-linux-musleabihf -mcpu=cortex_a9 -static -O2 stub.c -o build/bin/homekey-stub

cp homekey.png build/shared/res/homekey.png
cp tizen-manifest.xml build/tizen-manifest.xml
rm -f homekey.tpk
(cd build && zip -qr ../homekey.tpk .)
echo "built homekey.tpk"
