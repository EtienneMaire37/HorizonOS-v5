export PREFIX="$(pwd)/i486elfgcc"
export TARGET=i486-elf
export PATH="$PREFIX/bin:$PATH"

set -x -e

mkdir -p "$PREFIX"

mkdir -p ./tmp
cd ./tmp
rm -rf ./*

wget https://ftpmirror.gnu.org/gnu/binutils/binutils-2.44.tar.gz
tar xf binutils-2.44.tar.gz

mkdir -p binutils-build
cd binutils-build
../binutils-2.44/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

cd ..

wget https://ftpmirror.gnu.org/gcc/gcc-15.1.0/gcc-15.1.0.tar.gz

tar xf gcc-15.1.0.tar.gz

mkdir -p gcc-build
cd gcc-build
../gcc-15.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3

echo "Installed binaries:"
ls "$PREFIX/bin" -l