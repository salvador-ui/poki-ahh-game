#!/bin/bash
# Builds the sh3eb-elf cross toolchain + mkg3a + libfxcg for the Casio fx-CG50 (PrizmSDK layout) on macOS.
set -euo pipefail
ROOT="$HOME/prizm-toolchain"; PREFIX="$ROOT/prefix"; SRC="$ROOT/src"; JOBS=$(sysctl -n hw.ncpu)
mkdir -p "$SRC" "$PREFIX"; cd "$SRC"
BP=$(brew --prefix)
brew list gmp mpfr libmpc libpng cmake >/dev/null 2>&1 || brew install gmp mpfr libmpc libpng cmake
export PATH="$PREFIX/bin:$PATH"
export MAKEINFO=true   # no texinfo needed: skip building docs
BIN=2.41; GCC=14.2.0
[ -f binutils-$BIN.tar.xz ] || curl -fL -O https://ftp.gnu.org/gnu/binutils/binutils-$BIN.tar.xz
[ -f gcc-$GCC.tar.xz ]     || curl -fL -O https://ftp.gnu.org/gnu/gcc/gcc-$GCC/gcc-$GCC.tar.xz
[ -d binutils-$BIN ] || tar xf binutils-$BIN.tar.xz
[ -d gcc-$GCC ]      || tar xf gcc-$GCC.tar.xz
if [ ! -x "$PREFIX/bin/sh3eb-elf-as" ]; then
  mkdir -p build-binutils && cd build-binutils
  ../binutils-$BIN/configure --target=sh3eb-elf --prefix="$PREFIX" --disable-nls --disable-werror --with-sysroot --with-system-zlib
  make -j$JOBS && make install; cd ..
fi
if [ ! -x "$PREFIX/bin/sh3eb-elf-gcc" ]; then
  mkdir -p build-gcc && cd build-gcc
  ../gcc-$GCC/configure --target=sh3eb-elf --prefix="$PREFIX" --with-pkgversion=PrizmSDK \
    --without-headers --enable-languages=c --disable-tls --disable-nls --disable-threads --disable-shared \
    --disable-libssp --disable-libvtv --disable-libada --disable-libsanitizer --disable-bootstrap \
    --with-endian=big --with-multilib-list= --with-system-zlib --with-gmp="$BP" --with-mpfr="$BP" --with-mpc="$BP"
  make -j$JOBS MAKEINFO=true inhibit_libc=true all-gcc
  make MAKEINFO=true install-gcc
  cd ..
fi
if [ ! -f "$PREFIX/lib/gcc/sh3eb-elf/$GCC/libgcc.a" ]; then
  cd build-gcc
  make -j1 inhibit_libc=true all-target-libgcc      # -j1: parallel builds race on libgcc_tm.h
  make install-target-libgcc
  cd ..
fi
if [ ! -x "$PREFIX/bin/mkg3a" ]; then
  [ -d mkg3a ] || git clone --depth 1 https://gitlab.com/taricorp/mkg3a.git
  cd mkg3a && cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$BP" . && make -j$JOBS && make install; cd ..
fi
if [ ! -f "$ROOT/libfxcg/lib/libfxcg.a" ]; then
  [ -d "$ROOT/libfxcg" ] || git clone --depth 1 https://github.com/Jonimoose/libfxcg.git "$ROOT/libfxcg"
  (cd "$ROOT/libfxcg" && FXCGSDK="$ROOT/libfxcg" make)
fi
echo TOOLCHAIN_DONE
