export PREFIX="$(pwd)/i486elfgcc"
export TARGET=i486-elf
export PATH="$PREFIX/bin:$PATH"

mkdir $PREFIX -p

mkdir /tmp/src
cd /tmp/src

curl -O http://ftp.gnu.org/gnu/binutils/binutils-2.44.tar.gz
tar xf binutils-2.44.tar.gz

mkdir binutils-build
cd binutils-build
../binutils-2.44/configure --target=$TARGET --enable-interwork --enable-multilib --disable-nls --disable-werror --prefix=$PREFIX 2>&1 | tee configure.log
make all install 2>&1 | tee make.log

cd /tmp/src

curl -O https://ftp.gnu.org/gnu/gcc/gcc-15.1.0/gcc-15.1.0.tar.gz
tar xf gcc-15.1.0.tar.gz

mkdir gcc-build
cd gcc-build
../gcc-15.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-libssp --enable-language=c,c++ --without-headers

make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

echo "Installed binaries:"
ls "$PREFIX/bin"

export PATH="$PATH:$PREFIX/bin"