PREFIX=`pwd`/builds/roku

OUTDIR=builds/embedruntimes/roku

TOOLCHAIN="$NDKDIR/toolchain"
PLATFORM="$NDKDIR/platform"

CXXFLAGS="-mno-thumb -DARM_FPU_VFP=1 -D__ARM_EABI__ -Os -mcpu=arm1176jzf-s -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=vfp";

ROKU_LINK_LIBS=" \
        -L$PLATFORM/lib \
        -L$PLATFORM/usr/lib \
        -Wl,--as-needed \
        -Wl,-Bstatic  -lstdc++ \
        -Wl,-Bstatic  -lsupc++ \
        -Wl,-Bstatic  -lgcc \
        -Wl,-Bstatic  -lgcc_eh \
        -Wl,-Bdynamic  -lc \
        -Wl,-Bdynamic -lpthread \
        -Wl,-Bdynamic -lrt \
        -Wl,--no-as-needed"

LDFLAGS="-nodefaultlibs -Wl,--gc-sections $ROKU_LINK_LIBS"

PATH="$TOOLCHAIN/bin:$PATH"
CC="$TOOLCHAIN/bin/arm-linux-cc -nostdlib" #-nostdlib
CXX="$TOOLCHAIN/bin/arm-linux-c++ -nostdlib" #-nostdlib
CPP="$TOOLCHAIN/bin/arm-linux-cpp"
CXXCPP="$TOOLCHAIN/bin/arm-linux-cpp"
CPATH="$PLATFORM/usr/include"
LD=$TOOLCHAIN/bin/arm-linux-ld
AS=$TOOLCHAIN/bin/arm-linux-as
AR=$TOOLCHAIN/bin/arm-linux-ar
RANLIB=$TOOLCHAIN/bin/arm-linux-ranlib
STRIP=$TOOLCHAIN/bin/arm-linux-strip

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=roku_cross.cache \
--host=arm-brcm-linux-gnueabi \
--disable-mcs-build \
--disable-parallel-mark \
--disable-shared-handles \
--with-sigaltstack=no \
--with-tls=pthread \
--with-glib=embedded \
--disable-nls \
mono_cv_uscore=yes"

make clean && make distclean
rm roku_cross.cache

pushd eglib
autoreconf -i
popd
autoreconf -i

# Run configure
./configure $CONFIG_OPTS CFLAGS="$CXXFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"

# Run Make
make && echo "Build SUCCESS!" || exit 1

rm -rf builds

mkdir -p $OUTDIR	
cp -f mono/mini/.libs/libmono.a $OUTDIR
cp -f mono/mini/.libs/libmono.so $OUTDIR

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi



