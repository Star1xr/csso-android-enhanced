#!/bin/sh
# Output: output/armeabi-v7a/*.so
. scripts/android-setup.sh
./waf configure -T release --build-games=csso --prefix=$PWD/output/armeabi-v7a --android=armeabi-v7a-hard,host,21 --32bits --enable-opus --dxvk --disable-warns $WAF_EXTRA &&
./waf install --strip -k
