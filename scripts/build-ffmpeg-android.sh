#!/bin/sh
# Builds the FFmpeg 6.1 shared libraries video_bink links against for one Android
# ABI and drops them into lib/android/<cpu>/ (next to the other prebuilts).
#
# usage: scripts/build-ffmpeg-android.sh armeabi-v7a|arm64-v8a
#
# Skips the build when the libs are already there, so it is safe to call on
# every run (CI caches ffmpeg-out-<cpu> to make it a no-op).
set -e

ABI=$1
case "$ABI" in
	armeabi-v7a)
		CPU=arm
		ARCH=arm
		# keep in sync with scripts/waifulib/xcompile.py (armeabi-v7a-hard)
		TRIPLE=arm-linux-androideabi21
		GCC_TOOLCHAIN=arm-linux-androideabi-4.9
		CPUFLAGS="-march=armv7-a -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
		EXTRALIBS="-lm_hard"
		;;
	arm64-v8a)
		CPU=aarch64
		ARCH=aarch64
		TRIPLE=aarch64-linux-android21
		GCC_TOOLCHAIN=aarch64-linux-android-4.9
		CPUFLAGS=""
		EXTRALIBS=""
		;;
	*)
		echo "usage: $0 armeabi-v7a|arm64-v8a" >&2
		exit 1
		;;
esac

DEST=lib/android/$CPU
FFMPEG_LIBS="avcodec avformat avutil avdevice avfilter swresample swscale"

copy_libs() {
	mkdir -p "$DEST"
	for lib in $FFMPEG_LIBS; do
		cp -L "$1/lib/lib$lib.so" "$DEST/"
	done
	ls -l "$DEST"
}

if [ -f "$DEST/libavcodec.so" ]; then
	echo "$DEST already contains ffmpeg, nothing to do"
	exit 0
fi

. scripts/android-setup.sh

NDK=$PWD/android-ndk-r10e
LLVM=$PWD/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04
SYSROOT=$NDK/platforms/android-21/arch-$CPU
GCC=$NDK/toolchains/$GCC_TOOLCHAIN/prebuilt/linux-x86_64
SRC=ffmpeg-6.1
OUT=$PWD/ffmpeg-out-$CPU

CFLAGS="--target=$TRIPLE --sysroot=$SYSROOT --gcc-toolchain=$GCC $CPUFLAGS"
LDFLAGS="--target=$TRIPLE --sysroot=$SYSROOT --gcc-toolchain=$GCC -fuse-ld=lld $EXTRALIBS"

if [ -f "$OUT/lib/libavcodec.so" ]; then
	echo "reusing cached ffmpeg build in $OUT"
	copy_libs "$OUT"
	exit 0
fi

if [ ! -d "$SRC" ]; then
	wget -q "https://ffmpeg.org/releases/$SRC.tar.xz"
	tar -xf "$SRC.tar.xz"
fi

cd "$SRC"
./configure \
	--prefix="$OUT" \
	--target-os=android \
	--enable-cross-compile \
	--arch=$ARCH \
	--cc="$LLVM/bin/clang" \
	--cxx="$LLVM/bin/clang++" \
	--ar="$LLVM/bin/llvm-ar" \
	--nm="$LLVM/bin/llvm-nm" \
	--ranlib="$LLVM/bin/llvm-ranlib" \
	--strip="$LLVM/bin/llvm-strip" \
	--sysroot="$SYSROOT" \
	--extra-cflags="$CFLAGS -fPIC" \
	--extra-ldflags="$LDFLAGS" \
	--enable-pic \
	--enable-shared --disable-static \
	--disable-programs --disable-doc --disable-postproc \
	--disable-everything \
	--enable-decoder=bink,binkaudio_dct,binkaudio_rdft \
	--enable-demuxer=bink \
	--enable-protocol=file \
	--enable-avdevice --enable-avfilter --enable-swresample --enable-swscale \
	|| { echo "ffmpeg configure failed"; tail -n 120 ffbuild/config.log; exit 1; }

make -j"$(nproc)"
make install

cd ..
copy_libs "$OUT"
