PREFIX=`pwd`/builds/leon-mtk

OUTDIR=builds/embedruntimes/leon-mtk

CXXFLAGS="-Os -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfpu=vfpv3-d16 -mtune=cortex-a9";
CC="armv7a-mediatek451_001_vfp-linux-gnueabi-gcc"
CXX="armv7a-mediatek451_001_vfp-linux-gnueabi-g++"
AR="armv7a-mediatek451_001_vfp-linux-gnueabi-ar"
LD="armv7a-mediatek451_001_vfp-linux-gnueabi-ld"
LDFLAGS=""

CONFIG_OPTS="\
--prefix=$PREFIX \
--cache-file=leon_mtk_cross.cache \
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
rm leon_cross_mtk.cache

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



