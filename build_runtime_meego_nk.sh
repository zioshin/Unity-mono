#!/bin/bash

PREFIX="$PWD/builds/meego-armv7"

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

export LDFLAGS CFLAGS CXXFLAGS

/scratchbox/login -k -d $BUILDDIR make clean && make distclean
/scratchbox/login -k -d $BUILDDIR rm meego_cross.cache

pushd eglib
/scratchbox/login -k -d $BUILDDIR autoreconf -i
popd
/scratchbox/login -k -d $BUILDDIR autoreconf -i

# Run configure
/scratchbox/login -k -d $BUILDDIR ./configure $CONFIG_OPTS 

# Run Make
/scratchbox/login -k -d $BUILDDIR make && echo "Build SUCCESS!" || exit 1

rm -rf $PWD/builds

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.so $OUTDIR

# Clean up for next build
/scratchbox/login -k -d $BUILDDIR make clean && make distclean

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi



