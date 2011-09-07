PREFIX=`pwd`/builds/meego

OUTDIR=builds/embedruntimes/meego

CXXFLAGS="-O2 -DARM_FPU_VFP=1 -D__ARM_EABI__ -mno-thumb -march=armv7-a -mfloat-abi=hard -mfpu=vfpv3 -mtune=cortex-a8 -lsoftlibm";
CFLAGS="$CXXFLAGS"

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

/scratchbox/login -k -d /$PWD make clean && make distclean

/scratchbox/login -k -d /$PWD rm meego_cross.cache

pushd eglib
/scratchbox/login -k -d /$PWD autoreconf -i
popd
/scratchbox/login -k -d /$PWD autoreconf -i

# Run configure
/scratchbox/login -k -d /$PWD ./configure $CONFIG_OPTS CFLAGS="$CXXFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS"

# Run Make
/scratchbox/login -k -d /$PWD make && echo "Build SUCCESS!" || exit 1

rm -rf $OUTDIR

mkdir -p $OUTDIR	
cp -f mono/mini/.libs/libmono.so $OUTDIR

# Clean up for next build
/scratchbox/login -k -d /$PWD make clean && make distclean
