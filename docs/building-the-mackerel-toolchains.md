# Building the Mackerel Toolchain(s)

Mackerel code and the Linux image (at least on Mackerel-30) are compiled with custom toolchains. These are created using crosstools-ng. There are two defconfigs provided in this repository, one for the baremetal toolchain (bootloader, test apps) and one for the Linux toolchain (v6.18.x with musl libc).

1. Run one of the `tools/install_reqs_` scripts based on your Linux distribution.

2. Compile crosstools-ng:

```
git clone https://github.com/crosstool-ng/crosstool-ng.git
cd crosstool-ng
./bootstrap
./configure --prefix=/home/$(whoami)/crosstools
make
make install
```

3. Use crosstools-ng to build a toolchain for Mackerel:

```
cd <some_empty_workspace_directory>

# Make sure the newly built crosstools-ng binary is available in the path
export PATH=$PATH:/home/$(whoami)/crosstools/bin

# Pick the defconfig based on whether you want the baremetal toolchain or the Linux toolchain
# You can build both if needed.
cp <path_to_this_repo>/tools/<mackerel_defconfig_file> defconfig

# Create a full .config from the simplified defconfig
ct-ng defconfig

# Build the Mackerel toolchain, this can take a while (5-20 minutes)
ct-ng build
```

4. Verify the compiler works:

```
~/x-tools/m68k-mackerel-linux-musl/bin/m68k-mackerel-linux-musl-gcc --version

m68k-mackerel-linux-musl-gcc (crosstool-NG 1.28.0.43_1193ab8) 16.1.0
Copyright (C) 2026 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```