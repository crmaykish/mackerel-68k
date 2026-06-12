#!/usr/bin/env bash
# Clone the external soft cores used by Mackerel-F

set -euo pipefail

# Clone into the cores directory regardless of where this script is called from.
CORES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/cores"

echo "Cloning fx68k into $CORES_DIR/fx68k..."
git clone https://github.com/ijor/fx68k.git "$CORES_DIR/fx68k"

echo "Done."
