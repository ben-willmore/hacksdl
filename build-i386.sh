#!/usr/bin/env bash
BUILD_DATE=`date +"%Y%m%d_%H%M%S"`
GIT_HASH=`git rev-parse HEAD`
GIT_BRANCH=`git branch --show-current`
VERSION="${1:-TESTING}"
hwplatform=i386
[[ "${hwplatform}" == "aarch64" ]] && PATHLIB="/usr/lib/aarch64-linux-gnu" && ARCH="aarch64"
[[ "${hwplatform}" == "armv7l" ]] && PATHLIB="/usr/lib/arm-linux-gnueabihf" && ARCH="armhf"
[[ "${hwplatform}" == "i386" ]] && PATHLIB="/usr/lib/i386-linux-gnu" && ARCH="i386"

gcc hacksdl.c configuration.c debug.c -m32 "${PATHLIB}/libconfig.a" -o "hacksdl.${ARCH}.so" -m32 -fPIC -shared -lSDL2 -D_GNU_SOURCE -Wl,--defsym,BUILD_DATE_${BUILD_DATE}=0 -Wl,--defsym,GIT_HASH_${GIT_HASH}=0 -Wl,--defsym,GIT_BRANCH_${GIT_BRANCH}=0 -Wl,--defsym,VERSION_${VERSION}=0

strip "hacksdl.${ARCH}.so"
cp "hacksdl.${ARCH}.so" "hacksdl.so"
zip "hacksdl-${ARCH}-${VERSION}.zip" "hacksdl.so"
rm "hacksdl.so"
