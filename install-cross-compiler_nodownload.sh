export PREFIX="$(pwd)/i486elfgcc"
export TARGET=i486-elf
export PATH="$PREFIX/bin:$PATH"

set -x -e

mkdir -p "$PREFIX"

mkdir -p /tmp/src
cd /tmp/src

mkdir -p binutils-build
cd binutils-build
../binutils-2.44/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

cd /tmp/src

mkdir -p gcc-build
cd gcc-build
../gcc-15.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx -enable-multilib
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3

echo "Installed binaries:"
ls "$PREFIX/bin" -l