## Building Linux v6.18 for Mackerel-30

```
# Clone/download the Linux source from: https://github.com/crmaykish/mackerel-linux
git clone https://github.com/crmaykish/mackerel-linux

# Checkout the Mackerel-30 branch
cd mackerel-linux
git checkout mackerel-30

# Build the kernel image
./build_kernel_full.sh

# Build busybox
./build_busybox.sh

# Build the CF card (or other IDE-compatible drive)
# Replace /dev/sdX with your CF card device name
sudo ./makeroot.sh /dev/sdX

# Plug in the CF card and run the `ide` command from the bootloader
```
