FROM debian:bullseye

RUN apt-get update

# Install native build tools from the repo
RUN apt-get install -y build-essential flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo 

# Install other crap
RUN apt-get install -y wget libncurses5-dev bc genromfs

# Setup a dev user
RUN useradd -ms /bin/bash mackerel
USER mackerel
WORKDIR /home/mackerel

ENV PREFIX="/home/mackerel/opt/cross"
ENV TARGET=m68k-elf
ENV PATH="$PREFIX/bin:$PATH"

# Download the build tools source
RUN wget -q http://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.gz


# Download and build binutils
RUN tar xzf binutils-2.38.tar.gz
WORKDIR binutils-2.38
RUN mkdir build
WORKDIR build
RUN ../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
RUN make -j $(nproc)
RUN make install

WORKDIR /home/mackerel/

# Download and build gcc
RUN wget -q http://ftp.gnu.org/gnu/gcc/gcc-11.3.0/gcc-11.3.0.tar.gz
RUN tar xvf gcc-11.3.0.tar.gz
WORKDIR gcc-11.3.0
RUN mkdir build
WORKDIR build
RUN ../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
RUN make -j $(nproc) all-gcc
RUN make -j $(nproc) all-target-libgcc
RUN make install-gcc
RUN make install-target-libgcc

WORKDIR /home/mackerel/project/

# TODO: build and install newlib for bare metal apps