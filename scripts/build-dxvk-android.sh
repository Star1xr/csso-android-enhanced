#!/bin/sh
# Builds libdxvk_d3d9.so (DXVK native, D3D9 only) for one ABI and drops it into
# lib/android/<cpu>/ next to libSDL2.so, where the engine's waf build looks for it.
#
# usage: scripts/build-dxvk-android.sh <arm64-v8a|armeabi-v7a>
#
# Needs: meson, ninja, glslangValidator, python3 and a modern NDK (r23+). The engine itself
# is built with NDK r10e, so DXVK uses its own NDK (GitHub runners ship one).
set -eu

ABI="${1:?usage: $0 <arm64-v8a|armeabi-v7a>}"
case "$ABI" in
	arm64-v8a)   CPU=aarch64 ;;
	armeabi-v7a) CPU=arm ;;
	*) echo "unsupported ABI $ABI" >&2; exit 1 ;;
esac

DXVK_REPO="${DXVK_REPO:-https://github.com/Star1xr/dxvk-native-android}"
DXVK_REF="${DXVK_REF:-master}"
DXVK_DIR="${DXVK_DIR:-thirdparty/dxvk-native-android}"

if [ ! -d "$DXVK_DIR/.git" ]; then
	rm -rf "$DXVK_DIR"
	git clone --depth 1 ${DXVK_REF:+--branch "$DXVK_REF"} "$DXVK_REPO" "$DXVK_DIR"
fi

# modern NDK, not the r10e one used for the engine
export ANDROID_NDK_HOME="${DXVK_NDK:-${ANDROID_NDK_LATEST_HOME:-${ANDROID_NDK_ROOT:-}}}"
[ -n "$ANDROID_NDK_HOME" ] || { echo "no modern NDK found (set DXVK_NDK)" >&2; exit 1; }

# SDL headers + the libSDL2.so the launcher ships (DXVK must talk to the same SDL instance)
export SDL2_INCLUDE="$PWD/thirdparty/SDL"
export SDL2_LIB_DIR="$PWD/lib/android/$CPU"
[ -f "$SDL2_LIB_DIR/libSDL2.so" ] || { echo "missing $SDL2_LIB_DIR/libSDL2.so (submodules checked out?)" >&2; exit 1; }

"$DXVK_DIR/android/build-android.sh" "$ABI" 24 "$PWD/$DXVK_DIR/build-android"

mkdir -p "lib/android/$CPU"
cp -v "$DXVK_DIR/build-android/$ABI/lib/libdxvk_d3d9.so" "lib/android/$CPU/"
