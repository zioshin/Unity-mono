#!/bin/bash

# do both h13 and m13 build if there are no arguments
# do either build if first argument is h13 or m13

build() {
if [ "${LEON13_VER}" = "h13" ]; then
    echo "Building LEON13 h13"
    ARCH=arm
    PLATFORM=lg115x
    TOOLCHAIN=h13
    TUNE="-march=armv7-a -mtune=cortex-a9 -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad -ftree-vectorize"
else
    echo "Building LEON13 m13"
    ARCH=armv7a
    PLATFORM=mediatek451_001_vfp
    TOOLCHAIN=m13
    TUNE="-march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon"
fi

LEON13_TOOLCHAIN="${LEON13_SDK}/${TOOLCHAIN}/toolchain/bin/${ARCH}-${PLATFORM}-linux-gnueabi-"

PREFIX=`pwd`/builds/leon13/${TOOLCHAIN}

OUTDIR=builds/embedruntimes/leon13/${TOOLCHAIN}

CXXFLAGS="-Os -DHAVE_ARMV6=1 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb ${TUNE}";
CC="${LEON13_TOOLCHAIN}gcc"
CXX="${LEON13_TOOLCHAIN}g++"
AR="${LEON13_TOOLCHAIN}ar"
LD="${LEON13_TOOLCHAIN}ld"
LDFLAGS=""

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=leon_cross.cache \
--host=arm-unknown-linux-gnueabi \
--disable-mcs-build \
--disable-parallel-mark \
--disable-shared-handles \
--with-sigaltstack=no \
--with-tls=pthread \
--with-glib=embedded \
--disable-nls \
mono_cv_uscore=yes"

make clean && make distclean
rm leon_cross.cache

pushd eglib
autoreconf -i
popd
autoreconf -i

# Run configure
./configure $CONFIG_OPTS CFLAGS="$CXXFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" CC="$CC" CXX="$CXX" AR="$AR" LD="$LD"

# Run Make
make && echo "Build SUCCESS!" || exit 1

rm -rf $OUTDIR 

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.a $OUTDIR

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi
}

rm -rf builds
if [ $# -eq 0 -o "$1" = "h13" ]; then
	LEON13_VER=h13
	build
fi

if [ $# -eq 0 -o "$1" = "m13" ]; then
	LEON13_VER=m13
	build
fi
