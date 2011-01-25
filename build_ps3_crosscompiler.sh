#!/bin/sh

echo Configuring build...
./autogen.sh --target=powerpc64-ps3-linux-gnu --disable-nls --disable-mcs-build --with-gc=bohem --disable-embed-check --with-libgc-threads=win32 --with-profile4=yes --host=i586-mingw32msvc || exit 1

echo Building...
make || exit 1

rm -rf builds
mkdir builds
mkdir builds/crosscompiler
mkdir builds/crosscompiler/ps3

echo Copying build result
cp mono/mini/.libs/mono.exe builds/crosscompiler/ps3/mono-xcompiler.exe || exit 1
cp mono/mini/.libs/libmono-2.0.dll builds/crosscompiler/ps3 || exit 1

