## Building Linux v6.13 for Mackerel-30

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
