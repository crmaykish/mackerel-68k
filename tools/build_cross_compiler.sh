# This script will build a cross-compiler for Mackerel. It uses a modern gcc and binutils version.
# I tested this script on Debian 12 and Arch.
# Note: the cross-compiler built by this script will build the bootloader and other bare-metal programs.
# It will not build Linux/uClinux.

#!/bin/sh

set -e

export PREFIX="/home/$(whoami)/mackerel/cross"
export TARGET=m68k-mackerel-elf
export PATH="$PREFIX/bin:$PATH"

BINUTILS_VERSION="2.44"
GCC_VERSION="14.2.0"

log_message() {
    echo ""
    echo "======================================="
    echo "$1"
    echo "======================================="
}

rm -rf binutils* gcc* Baselibc

# Download binutils
log_message "Downloading binutils version ${BINUTILS_VERSION}"
wget -q "http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz"

log_message "Building binutils ${BINUTILS_VERSION}"

tar xzf "binutils-${BINUTILS_VERSION}.tar.gz"
cd "binutils-${BINUTILS_VERSION}"
mkdir build
cd build
../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j $(nproc)
make install

log_message "Installed binutils ${BINUTILS_VERSION} to $PREFIX"

cd ../../


# Download and build gcc
log_message "Downloading gcc version ${GCC_VERSION}"
wget -q "http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz"

log_message "Building gcc ${GCC_VERSION}"
tar xzf "gcc-${GCC_VERSION}.tar.gz"
cd "gcc-${GCC_VERSION}"
mkdir build
cd build
../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make -j $(nproc) all-gcc
make -j $(nproc) all-target-libgcc
make install-gcc
make install-target-libgcc

log_message "Installed gcc ${GCC_VERSION} to $PREFIX"

cd ../../

log_message "Installing Baselibc"

git clone https://github.com/crmaykish/Baselibc.git
cd Baselibc

make PLATFORM=mackerel
cp -r include/ "${PREFIX}/${TARGET}/"
cp libc.a "${PREFIX}/${TARGET}/lib/"

log_message "Installed Baselibc"

cd ../

log_message "Cleaning up"

rm -rf binutils* gcc* Baselibc

log_message "Done! Add ${PREFIX}/bin to your PATH to use the ${TARGET} tools."

set +e
