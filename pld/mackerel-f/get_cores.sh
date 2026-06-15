#!/usr/bin/env bash
# Clone the external soft cores used by Mackerel-F

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

# fx68k 68000 soft core
clone_core fx68k https://github.com/ijor/fx68k.git

# OpenCores 16550-compatible UART (freecores GitHub mirror)
clone_core uart16550 https://github.com/freecores/uart16550.git

echo "Done."
