# This script will build a cross-compiler for Mackerel. It uses a modern gcc and binutils version.
# I tested this script on Debian 12. It will probably work on similar Debian-based distros.
# Note: this compiler will build the bootloader and other bare-metal programs. It does not build uClinux.

export PREFIX="/home/$(whoami)/opt/cross"
export TARGET=m68k-elf
export PATH="$PREFIX/bin:$PATH"

BINUTILS_VERSION="2.38"
GCC_VERSION="11.2.0"

log_message() {
    echo ""
    echo "======================================="
    echo "$1"
    echo "======================================="
}

rm -rf binutils* gcc* Baselibc

# Install build dependencies
log_message "Installing dependencies from apt repo"
sudo apt-get update
sudo apt-get install -y build-essential flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo wget libncurses5-dev bc genromfs unzip git

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
make PLATFORM=m68k-elf
cp -r include/ "${PREFIX}/m68k-elf/"
cp libc.a "${PREFIX}/m68k-elf/lib/"

log_message "Installed Baselibc"

cd ../

log_message "Cleaning up"

rm -rf binutils* gcc* Baselibc

log_message "Done! Add ${PREFIX}/bin to your PATH to use the m68k-elf- tools."
