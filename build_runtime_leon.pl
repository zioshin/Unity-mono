PREFIX=`pwd`/builds/leon

OUTDIR=builds/embedruntimes/leon

CXXFLAGS="-g -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfpu=vfpv3 -mtune=cortex-a9";
CC="arm-none-linux-gnueabi-gcc"
CXX="arm-none-linux-gnueabi-g++"
AR="arm-none-linux-gnueabi-ar"
LD="arm-none-linux-gnueabi-ld"
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
cp -f mono/mini/.libs/libmono.so $OUTDIR

rm -r builds/monodistribution


