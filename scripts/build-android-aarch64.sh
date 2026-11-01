#!/bin/sh
. scripts/android-setup.sh
./waf configure -T release --build-game=cstrike --prefix=Srceng-mod-launcher/android --android=aarch64,host,21 --target=../aarch64 --enable-opus --togles --disable-warns $WAF_EXTRA &&
./waf install --target=client,server,engine,vgui2,vguimatsurface,vstdlib$EXTRA_TARGETS --strip
status=$?

if [ -e "Srceng-mod-launcher/android/lib/arm64-v8a/README.md" ]; then
	rm Srceng-mod-launcher/android/lib/arm64-v8a/README.md
fi

exit $status
