## Building Linux v6.13 for Mackerel-30

### Setting up a toolchain

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

# Copy the toolchain defconfig from this repo into the workspace
cp <path_to_this_repo>/tools/mackerel_crosstools_defconfig defconfig

# Create a full .config from the simplified defconfig
ct-ng defconfig

# Build the Mackerel toolchain, this can take a while (5-20 minutes)
ct-ng build
```

4. Verify the build:

```
~/x-tools/m68k-mackerel-linux-gnu/bin/m68k-mackerel-linux-gnu-gcc --version

m68k-mackerel-linux-gnu-gcc (crosstool-NG 1.27.0.20_329bb4d) 14.2.0
Copyright (C) 2024 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

### Building the Linux kernel for Mackerel-30

```
cd <mackerel_linux_mmu_repo>
git checkout mackerel-30-config

# Put the toolchain on the path
export PATH=$PATH:/home/$(whoami)/x-tools/m68k-mackerel-linux-gnu/bin

# Load the Mackerel-30 config
make ARCH=m68k mackerel30_defconfig

# Compile the kernel
make ARCH=m68k CROSS_COMPILE=m68k-mackerel-linux-gnu- -j$(nproc)
```
