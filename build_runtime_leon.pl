PREFIX=`pwd`/builds/leon

OUTDIR=builds/embedruntimes/leon

CXXFLAGS="-Os -DHAVE_ARMV6=1 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfpu=vfpv3 -mtune=cortex-a9";
CC="arm-lg1152-linux-gnueabi-gcc"
CXX="arm-lg1152-linux-gnueabi-g++"
AR="arm-lg1152-linux-gnueabi-ar"
LD="arm-lg1152-linux-gnueabi-ld"
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

rm -rf builds

mkdir -p $OUTDIR
cp -f mono/mini/.libs/libmono.a $OUTDIR

if [ -d builds/monodistribution ] ; then
rm -r builds/monodistribution
fi



