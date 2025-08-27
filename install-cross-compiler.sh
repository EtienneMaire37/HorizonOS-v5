export PREFIX="$(pwd)/i486elfgcc"
export TARGET=i486-elf
export PATH="$PREFIX/bin:$PATH"

set -x -e

mkdir -p "$PREFIX"

mkdir -p /tmp/src
cd /tmp/src

curl -O http://ftp.gnu.org/gnu/binutils/binutils-2.44.tar.gz
tar xf binutils-2.44.tar.gz

mkdir -p binutils-build
cd binutils-build
../binutils-2.44/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make all
make install

cd /tmp/src

curl -O https://ftp.gnu.org/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.gz
tar xf gcc-15.1.0.tar.gz

mkdir -p gcc-build
cd gcc-build
../gcc-15.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx

make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

echo "Installed binaries:"
ls "$PREFIX/bin"