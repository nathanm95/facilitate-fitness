#!/bin/sh
# Build, install and screenshot in one go.
#   FF_FAKE=1 FF_SHOW_TIME=1008 tools/snap.sh /tmp/x emery gabbro
# writes /tmp/x_emery.png and /tmp/x_gabbro.png. FF_* flags pass straight
# through to the build (see wscript).
set -e
cd "$(dirname "$0")/.."
out=$1
shift
pebble build >/dev/null
for p in "$@"; do
  pebble install --emulator "$p" 2>&1 | grep -E "succeeded|rror" || true
  sleep 3
  python3 tools/shot.py "$p" "${out}_$p.png"
done
