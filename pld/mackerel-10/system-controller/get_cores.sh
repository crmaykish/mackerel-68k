#!/usr/bin/env bash

set -euo pipefail

# Clone into the cores directory regardless of where this script is called from.
CORES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cores"

# clone_core <name> <url> -- clone into $CORES_DIR/<name>, skip if already present
clone_core() {
    local name="$1" url="$2" dest="$CORES_DIR/$1"
    if [ -d "$dest" ]; then
        echo "Skipping $name (already present at $dest)"
        return
    fi
    echo "Cloning $name into $dest..."
    git clone "$url" "$dest"
}

# OpenCores tiny_spi SPI master (instantiated by spi.v as the W5500/SPI master)
clone_core tiny_spi https://github.com/freecores/tiny_spi.git

echo "Done."
