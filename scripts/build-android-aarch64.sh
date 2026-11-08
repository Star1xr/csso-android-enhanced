#!/bin/sh
# Output: output/arm64-v8a/*.so
. scripts/android-setup.sh
./waf configure -T release --build-games=csso --prefix=$PWD/output/arm64-v8a --android=aarch64,host,21 --enable-opus --togles --disable-warns $WAF_EXTRA &&
./waf install --target=client,server,engine,vgui2,vguimatsurface,vstdlib,scenefilecache --strip
