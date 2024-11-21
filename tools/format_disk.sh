#!/bin/bash

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run with sudo privileges."
    exit 1
fi

if [ -z "$1" ]; then
    echo "Usage: $0 <drive>"
    exit 1
fi

drive="$1"

echo "Warning! This will erase all data on the $drive"
read -p "Proceed? (yes/no): " confirm

if [[ "$confirm" != "yes" && "$confirm" != "y" ]]; then
    echo "Exiting."
    exit 1
fi

# Create a new MS-DOS partition table
parted --script $drive mklabel msdos

# Create a 64 MB FAT16 partition
parted --script $drive mkpart primary fat16 0% 64MB

# Create a 1 GB ext2 partition
parted --script $drive mkpart primary ext2 64MB 1064MB

mkfs.fat -F 16 ${drive}1
mkfs.ext2 ${drive}2

fdisk -l $drive

echo "Formatting completed on: $drive"
