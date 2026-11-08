#!/bin/sh
# Sourced by build-android-*.sh from the repo root. Safe to run repeatedly.
git submodule init && git submodule update

NDK=android-ndk-r10e
LLVM=clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04

if [ ! -d "$NDK" ]; then
	wget -q https://dl.google.com/android/repository/$NDK-linux-x86_64.zip
	unzip -q $NDK-linux-x86_64.zip && rm -f $NDK-linux-x86_64.zip
fi
if [ ! -d "$LLVM" ]; then
	wget -q https://github.com/llvm/llvm-project/releases/download/llvmorg-11.1.0/$LLVM.tar.xz
	tar -xf $LLVM.tar.xz && rm -f $LLVM.tar.xz
fi

chmod +x waf
export ANDROID_NDK_HOME=$PWD/$NDK/
export NDK_HOME=$PWD/$NDK/
export PATH="$PWD/$LLVM/bin:$PATH"
