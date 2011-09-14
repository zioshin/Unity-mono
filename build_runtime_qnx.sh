#!/bin/sh

# Set up QNX dev environment
# Taken from/opt/bbndk/NativeSDK-env-1_0_7.2942.sh

if [ -z $UNITY_THISISABUILDMACHINE ] ; then
QNX_TARGET="/opt/bbndk/target-1_0_7.2942/qnx6"
QNX_HOST="/opt/bbndk/host-0.9.3/linux/x86"
QNX_CONFIGURATION="/opt/bbndk/install/qnx"
MAKEFLAGS="-I$QNX_TARGET/usr/include"
LD_LIBRARY_PATH="$QNX_HOST/usr/lib:$LD_LIBRARY_PATH"
PATH="$QNX_HOST/usr/bin:$QNX_CONFIGURATION/bin:$QNX_HOST/usr/qde/eclipse/jre/bin:$PATH"
fi

export QNX_TARGET QNX_HOST QNX_CONFIGURATION MAKEFLAGS LD_LIBRARY_PATH PATH

PREFIX=`pwd`/../builds/qnx

OUTDIR=../builds/embedruntimes/qnx

make clean && make distclean

rm -r *.cache config.status nto-arm-le-v7 libgc/config.status autom4te.cache Makefile

NOCONFIGURE=1 ./autogen.sh
cd eglib; NOCONFIGURE=1 ./autogen.sh

cd ..
addvariant nto arm le-v7
cd nto-arm-le-v7

# Run Make
make && echo "Build SUCCESS!" || exit 1

rm -rf ../builds

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.so $OUTDIR

rm -r builds/monodistribution

# Clean up for next build
cd ..
make clean && make distclean
rm Makefile

