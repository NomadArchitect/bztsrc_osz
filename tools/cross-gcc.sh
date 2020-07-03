#!/bin/sh

#
# tools/cross-gcc.sh
#
# Copyright (c) 2017 bzt (bztsrc@gitlab)
# https://creativecommons.org/licenses/by-nc-sa/4.0/
#
# A művet szabadon:
#
# - Megoszthatod — másolhatod és terjesztheted a művet bármilyen módon
#     vagy formában
# - Átdolgozhatod — származékos műveket hozhatsz létre, átalakíthatod
#     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza
#     ezen engedélyeket míg betartod a licensz feltételeit.
#
# Az alábbi feltételekkel:
#
# - Nevezd meg! — A szerzőt megfelelően fel kell tüntetned, hivatkozást
#     kell létrehoznod a licenszre és jelezned, ha a művön változtatást
#     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve
#     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a
#     felhasználásod körülményeit.
# - Ne add el! — Nem használhatod a művet üzleti célokra.
# - Így add tovább! — Ha feldolgozod, átalakítod vagy gyűjteményes művet
#     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-
#     feltételek mellett kell terjesztened, mint az eredetit.
#
# @subsystem eszközök
# @brief Segédprogram a gcc keresztfordító letöltésére és lefordítására
#

### Konfiguráció
ARCHS="x86_64 aarch64"
DIR="/usr/local/gcc"
CFG=" --enable-shared --enable-threads=posix --enable-libmpx --with-system-zlib --with-isl --enable-__cxa_atexit \
--disable-libunwind-exceptions --enable-clocale=gnu --disable-libstdcxx-pch --disable-libssp --enable-plugin \
--disable-linker-build-id --enable-lto --enable-install-libiberty --with-linker-hash-style=gnu --with-gnu-ld\
--enable-gnu-indirect-function --disable-multilib --disable-werror --enable-checking=release --enable-default-pie \
--enable-default-ssp --enable-gnu-unique-object"

### A binutils és gcc forrásainak letöltése. A fájlnevekben a verziókat írd át. Máshol nem szerepel a verzió
[ ! -d tarballs ] && mkdir tarballs
echo "Downloading tarballs..."
cd tarballs
[ ! -f binutils-*tar* ] && wget http://ftpmirror.gnu.org/binutils/binutils-2.32.tar.gz
[ ! -f gcc-*tar* ] && wget http://ftpmirror.gnu.org/gcc/gcc-8.3.0/gcc-8.3.0.tar.gz
[ ! -f mpfr-*tar* ] && wget http://ftpmirror.gnu.org/mpfr/mpfr-4.0.2.tar.gz
[ ! -f gmp-*tar* ] && wget http://ftpmirror.gnu.org/gmp/gmp-6.1.2.tar.bz2
[ ! -f mpc-*tar* ] && wget http://ftpmirror.gnu.org/mpc/mpc-1.1.0.tar.gz
[ ! -f isl-*tar* ] && wget ftp://gcc.gnu.org/pub/gcc/infrastructure/isl-0.18.tar.bz2
[ ! -f cloog-*tar* ] && wget ftp://gcc.gnu.org/pub/gcc/infrastructure/cloog-0.18.1.tar.gz
cd ..

### Ez alatti sorokat ne módosítsd

### Letöltött fájlok kicsomagolása
echo -n "Unpacking tarballs... "
for i in tarballs/*.tar.gz; do d=${i%%.tar*};d=${d#*/}; echo -n "$d "; [ ! -d $d ] && tar -xzf $i; done
for i in tarballs/*.tar.bz2; do d=${i%%.tar*};d=${d#*/}; echo -n "$d "; [ ! -d $d ] && tar -xjf $i; done
echo "OK"

cd binutils-* && for i in isl; do ln -s ../$i-* $i 2>/dev/null || true; done && cd ..
cd gcc-* && for i in isl mpfr gmp mpc cloog; do ln -s ../$i-* $i 2>/dev/null || true; done && cd ..

# bugos gcc configure szkript javítása, hogy engedélyezzük a -fvisibility kapcsolót a gcc-ben
cd gcc-*/gcc
cat configure|sed 's/gcc_cv_as_hidden=no/gcc_cv_as_hidden=yes/'|sed 's/gcc_cv_ld_hidden=no/gcc_cv_ld_hidden=yes/'>..c
mv ..c configure
chmod +x configure
cd ../..

### Célkönyvtár létrehozása
#sudo mkdir -p $DIR
#sudo chown `whoami` $DIR
#rm -rf $DIR/*

### binutils és gcc lefordítása
for arch in $ARCHS; do
    echo "  -------------------------------------------- $arch -----------------------------------------------------"
    mkdir $arch-binutils 2>/dev/null
    cd $arch-binutils
    ../binutils-*/configure --prefix=$DIR --target=${arch}-elf $CFG
    make -j4
    make install
    cd ..
    mkdir $arch-gcc 2>/dev/null
    cd $arch-gcc
    ../gcc-*/configure --prefix=$DIR --target=${arch}-elf --enable-languages=c $CFG
    make -j4 all-gcc
    make install-gcc
    cd ..
done
