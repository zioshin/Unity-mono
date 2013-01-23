#!/bin/bash

PREFIX="$PWD/builds/tizen"

BUILDDIR=/$PWD
OUTDIR=builds/embedruntimes/tizen
CXXFLAGS="-Os -DHAVE_ARMV6=1 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3 -mtune=cortex-a9"
CFLAGS="$CXXFLAGS"

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=tizen_cross.cache \
--disable-mcs-build \
--disable-parallel-mark \
--disable-shared-handles \
--with-sigaltstack=no \
--with-tls=pthread \
--with-glib=embedded \
--disable-nls \
mono_cv_uscore=yes"

LDFLAGS="-L$BUILDDIR/unity"

${TIZEN_SB}/sb2 make clean && make distclean
${TIZEN_SB}/sb2 rm tizen_cross.cache

pushd eglib
${TIZEN_SB}/sb2 autoreconf -i
popd
${TIZEN_SB}/sb2 autoreconf -i

# Run configure
${TIZEN_SB}/sb2 ./configure $CONFIG_OPTS CFLAGS=\"$CXXFLAGS\" CXXFLAGS=\"$CXXFLAGS\" LDFLAGS=\"$LDFLAGS\"

# Run Make
${TIZEN_SB}/sb2 make && echo "Build SUCCESS!" || exit 1

rm -rf $PWD/builds

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.a $OUTDIR

# Clean up for next build
${TIZEN_SB}/sb2 make clean && make distclean

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi



