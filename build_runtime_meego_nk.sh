#!/bin/bash

PREFIX="$PWD/builds/meego"

if [ -z $UNITY_THISISABUILDMACHINE ] ; then
BUILDDIR=/$PWD
OUTDIR=builds/embedruntimes/meego-armv7
CXXFLAGS="-O2 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3 -mtune=cortex-a8 -lsoftlibm";
CFLAGS="$CXXFLAGS"

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=meego_cross.cache \
--host=arm-none-linux-gnueabi \
--disable-mcs-build \
--disable-parallel-mark \
--disable-shared-handles \
--with-sigaltstack=no \
--with-tls=pthread \
--with-glib=embedded \
--disable-nls \
mono_cv_uscore=yes"
else
BUILDDIR=/work/`basename $PWD`
unset TEMP
unset TMP
TEMP=/var/tmp
TMP=/var/tmp
export TEMP TMP
fi

LDFLAGS="-L$BUILDDIR/unity"

/scratchbox/login -d $BUILDDIR make clean && make distclean
/scratchbox/login -d $BUILDDIR rm meego_cross.cache

pushd eglib
/scratchbox/login -d $BUILDDIR autoreconf -i
popd
/scratchbox/login -d $BUILDDIR autoreconf -i

# Run configure
/scratchbox/login -d $BUILDDIR ./configure $CONFIG_OPTS CFLAGS="$CXXFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"

# Run Make
/scratchbox/login -d $BUILDDIR make && echo "Build SUCCESS!" || exit 1

rm -rf $PWD/builds

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.a $OUTDIR
cp -f unity/libsoftlibm.a $OUTDIR

# Clean up for next build
/scratchbox/login -d $BUILDDIR make clean && make distclean

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi



