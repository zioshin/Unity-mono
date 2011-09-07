PREFIX=`pwd`/builds/meego

OUTDIR=builds/embedruntimes/meego

CXXFLAGS="-O2 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3 -mtune=cortex-a8 -lsoftlibm";

LDFLAGS="-L`pwd`/unity"

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=meego_cross.cache \
--host=arm-unknown-linux-gnueabi \
--disable-mcs-build \
--disable-parallel-mark \
--disable-shared-handles \
--with-sigaltstack=yes \
--with-tls=pthread \
--with-glib=system \
--disable-nls \
mono_cv_uscore=yes"

make clean && make distclean
rm meego_cross.cache

pushd eglib
autoreconf -i
popd
autoreconf -i

# Run configure
./configure $CONFIG_OPTS CFLAGS="$CXXFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"

# Run Make
make && echo "Build SUCCESS!" || exit 1

rm -rf $OUTDIR

mkdir -p $OUTDIR	
cp -f mono/mini/.libs/libmono.a $OUTDIR
cp -f mono/mini/.libs/libmono.so $OUTDIR
