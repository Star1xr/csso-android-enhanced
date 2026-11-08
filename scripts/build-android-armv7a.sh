#!/bin/sh
# Output: output/armeabi-v7a/*.so
. scripts/android-setup.sh
./waf configure -T release --build-games=csso --prefix=$PWD/output/armeabi-v7a --android=armeabi-v7a-hard,host,21 --32bits --enable-opus --togles --disable-warns $WAF_EXTRA &&
./waf install --target=client,server,engine,vgui2,vguimatsurface,vstdlib,scenefilecache --strip
