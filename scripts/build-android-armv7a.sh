#!/bin/sh
. scripts/android-setup.sh
./waf configure -T release --build-game=csso --prefix=Srceng-mod-launcher/android --android=armeabi-v7a-hard,host,21 --target=../armeabi-v7a --32bits --enable-opus --togles --disable-warns $WAF_EXTRA &&
./waf install --target=client,server,engine,vgui2,vguimatsurface,vstdlib$EXTRA_TARGETS --strip
status=$?

if [ -e "Srceng-mod-launcher/android/lib/armeabi-v7a/README.md" ]; then
	rm Srceng-mod-launcher/android/lib/armeabi-v7a/README.md
fi

exit $status
