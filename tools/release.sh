#!/usr/bin/env bash
#
# Rebuilds every flashable artifact from source
#
# Artifacts:
#   mackerel-08-bootloader-*.bin
#   mackerel-08-{address,interrupt,dtack}-decoder-*.jed
#   mackerel-10-bootloader-{upper,lower}-*.bin
#   mackerel-30-bootloader-*.bin
#   mackerel-{10,30}-{system,dram}-controller-*.pof
#   mackerel-f-bootloader-*.bin + mackerel-f-*.fs

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$SCRIPT_DIR/.." && pwd)"
FW="$REPO/firmware"

export PATH="$PATH:$HOME/x-tools/m68k-mackerel-elf/bin"   # bare-metal toolchain
QUARTUS_MAP="$HOME/altera/13.0sp1/quartus/bin/quartus_map"
GWSH="$HOME/Gowin_EDA/IDE/bin/gw_sh"

# Version from the firmware, dated today; output always goes to dist/.
VERSION="$(grep -oE '#define VERSION "[^"]+"' "$FW/bootloader.c" | grep -oE '[0-9][^"]*')"
[ -z "$VERSION" ] && { echo "ERROR: could not read VERSION from bootloader.c"; exit 1; }
STAMP="v$VERSION-$(date +%Y-%m-%d)"
STAGE="$REPO/dist/mackerel-firmware-$STAMP"

STAGED=0
FAILED=0
have() { echo "  [ok]   $1"; STAGED=$((STAGED + 1)); }
fail() { echo "  [FAIL] $*" >&2; FAILED=$((FAILED + 1)); }

stage() {
    local src="$1" name="$2"
    if [ -f "$src" ] && install -m 0644 "$src" "$STAGE/$name"; then have "$name"; else fail "$name -- could not stage from $src"; fi
}

# Build one board/variant bootloader and stage its image(s).
#   $1 board id  $2 nn  $3 variant label  $4 make SRAM arg  $5 split(1=upper/lower)
build_bootloader() {
    local board="$1" nn="$2" variant="$3" sramarg="$4" split="$5"
    local vpart=""; [ -n "$variant" ] && vpart="-$variant"
    local targets="bootloader.bin"
    [ "$split" = 1 ] && targets="$targets bootloader-upper.bin bootloader-lower.bin"
    local log="/tmp/rel_${board}_${variant:-x}.log"
    echo "  building mackerel-$nn bootloader ${variant:+($variant) }..."
    if ! ( make -C "$FW" clean && make -C "$FW" BOARD="$board" $sramarg $targets ) >"$log" 2>&1; then
        fail "mackerel-$nn$vpart bootloader -- build failed (see $log)"
        return
    fi
    stage "$FW/bootloader.bin" "mackerel-$nn-bootloader$vpart-$STAMP.bin"
    if [ "$split" = 1 ]; then
        stage "$FW/bootloader-upper.bin" "mackerel-$nn-bootloader$vpart-upper-$STAMP.bin"
        stage "$FW/bootloader-lower.bin" "mackerel-$nn-bootloader$vpart-lower-$STAMP.bin"
    fi
}

# Build one Quartus CPLD project and stage its .pof.
#   $1 project dir (rel to repo)  $2 project/pof basename  $3 release name  $4 extra make args
build_cpld() {
    local dir="$1" proj="$2" name="$3" margs="${4:-}"
    if [ ! -x "$QUARTUS_MAP" ]; then
        fail "$name -- Quartus not found at $QUARTUS_MAP"; return
    fi
    local log="/tmp/rel_cpld_$(echo "$name" | tr -c 'A-Za-z0-9' _).log"
    echo "  building $name (Quartus)..."
    if ( make -C "$REPO/$dir" clean && make -C "$REPO/$dir" $margs ) >"$log" 2>&1; then
        stage "$REPO/$dir/output_files/$proj.pof" "$name"
    else
        fail "$name -- Quartus build failed (see $log)"
    fi
}

# Mackerel-08 GAL glue logic: three 22V10 JEDEC files from one galasm build.
build_gal() {
    if ! command -v galasm >/dev/null; then
        fail "mackerel-08 GAL logic -- galasm not found on PATH"; return
    fi
    local dir="$REPO/pld/mackerel-08"
    echo "  building mackerel-08 GAL logic (galasm)..."
    if ( make -C "$dir" clean && make -C "$dir" ) >/tmp/rel_gal.log 2>&1; then
        stage "$dir/address_decoder.jed"   "mackerel-08-address-decoder-$STAMP.jed"
        stage "$dir/interrupt_decoder.jed" "mackerel-08-interrupt-decoder-$STAMP.jed"
        stage "$dir/dtack_decoder.jed"     "mackerel-08-dtack-decoder-$STAMP.jed"
    else
        fail "mackerel-08 GAL logic -- galasm build failed (see /tmp/rel_gal.log)"
    fi
}

# Mackerel-F: bootloader image + full FPGA bitstream (bootloader baked in).
build_mackf() {
    echo "  building mackerel-f bootloader..."
    if ! ( make -C "$FW" clean && make -C "$FW" BOARD=mackf bootloader.hex ) >/tmp/rel_mackf.log 2>&1; then
        fail "mackerel-f bootloader -- build failed (see /tmp/rel_mackf.log)"; return
    fi
    stage "$FW/bootloader.bin" "mackerel-f-bootloader-$STAMP.bin"

    if [ ! -x "$GWSH" ]; then
        fail "mackerel-f bitstream -- Gowin (gw_sh) not found at $GWSH"; return
    fi
    echo "  synthesizing mackerel-f bitstream (Gowin)..."
    cp "$FW/bootloader.hex" "$REPO/pld/mackerel-f/rom.hex"
    if ( cd "$REPO/pld/mackerel-f" && PATH="$HOME/oss-cad-suite/bin:$PATH" make ) >/tmp/rel_mackf_synth.log 2>&1; then
        stage "$REPO/pld/mackerel-f/impl/pnr/mackerel_f.fs" "mackerel-f-$STAMP.fs"
    else
        fail "mackerel-f bitstream -- Gowin synth failed (see /tmp/rel_mackf_synth.log)"
    fi
}

# --- Build ------------------------------------------------------------------
echo "==> Mackerel-68k firmware+logic release $STAMP -> $STAGE"
rm -rf "$STAGE"; mkdir -p "$STAGE"

echo "-- bootloaders --"
build_bootloader mack08 08 "" "" 0
build_bootloader mack10 10 "" "" 1
build_bootloader mack30 30 "" "" 0

echo "-- glue logic --"
build_gal
# Mackerel-10 system controller: classic (68000/010, ÷2) vs SEC (68SEC000, 33 MHz)
build_cpld pld/mackerel-10/system-controller system-controller "mackerel-10-system-controller-classic-$STAMP.pof"
build_cpld pld/mackerel-10/system-controller system-controller "mackerel-10-system-controller-sec-$STAMP.pof" "SEC=yes"
build_cpld pld/mackerel-10/dram_controller   dram_controller   "mackerel-10-dram-controller-$STAMP.pof"
# Mackerel-30
build_cpld pld/mackerel-30/system_controller system_controller "mackerel-30-system-controller-$STAMP.pof"
build_cpld pld/mackerel-30/dram_controller   dram_controller   "mackerel-30-dram-controller-$STAMP.pof"

echo "-- Mackerel-F --"
build_mackf

if [ "$FAILED" -gt 0 ]; then
    echo
    echo "==================================================================="
    echo "  RELEASE ABORTED: $FAILED subtask(s) did not build cleanly."
    echo "  No release produced ($STAGED built artifact(s) discarded)."
    echo "  See the /tmp/rel_*.log files for details."
    echo "==================================================================="
    rm -rf "$STAGE"
    exit 1
fi

# --- README + checksums (only reached on a fully clean build) ---------------
cat >"$STAGE/README.md" <<'EOF'
# Mackerel 68k firmware release (@STAMP@)

Prebuilt, flashable artifacts for the [Mackerel 68k](https://github.com/crmaykish/mackerel-68k)
boards: bootloader ROM images, GAL/CPLD glue logic, and the Mackerel-F FPGA bitstream.

## Bootloaders

Flash the `.bin` images to the ROM chips with `minipro` or similar programmer.

| Board | Image | ROM chips |
|-------|-------|-----------|
| Mackerel-08 | `mackerel-08-bootloader-@STAMP@.bin` | one |
| Mackerel-10 | `mackerel-10-bootloader-{upper,lower}-@STAMP@.bin` | **two** |
| Mackerel-30 | `mackerel-30-bootloader-@STAMP@.bin` | one |
| Mackerel-F  | (in the FPGA bitstream, below) | — |

- **Mackerel-10 uses two ROMs** — the `-upper-`/`-lower-` halves; the non-suffixed image
  is the combined one (reference only).

## GAL glue logic (Mackerel-08)

Three GAL22V10 chips — flash the `.jed` files with `minipro`

| Chip | Image |
|------|-------|
| Address decoder   | `mackerel-08-address-decoder-@STAMP@.jed` |
| Interrupt decoder | `mackerel-08-interrupt-decoder-@STAMP@.jed` |
| DTACK decoder     | `mackerel-08-dtack-decoder-@STAMP@.jed` |

## CPLD glue logic (Mackerel-10 / Mackerel-30)

Flash the `.pof` images over JTAG with a USB Blaster (`quartus_pgm`, or the per-project
`make flash`):

| Board | Image | Notes |
|-------|-------|-------|
| Mackerel-10 | `mackerel-10-system-controller-classic-@STAMP@.pof` | **classic 68000/68010** (CPU ÷2) |
| Mackerel-10 | `mackerel-10-system-controller-sec-@STAMP@.pof` | **68SEC000 adapter** (full 33 MHz) |
| Mackerel-10 | `mackerel-10-dram-controller-@STAMP@.pof` | |
| Mackerel-30 | `mackerel-30-system-controller-@STAMP@.pof` | |
| Mackerel-30 | `mackerel-30-dram-controller-@STAMP@.pof` | |

Flashing the `sec` build on a real 68000/010 causes it run at 3x its rated speed. Don't do that. The DRAM controller is the
same for both.

## Mackerel-F (FPGA)

The whole system runs inside a Tang Nano 20k with the bootloader baked into the FPGA ROM,
so the release ships the **full bitstream** `mackerel-f-@STAMP@.fs` (the
`mackerel-f-bootloader-@STAMP@.bin` is the raw bootloader, for reference). Load with
[`openFPGALoader`](https://github.com/trabucayre/openFPGALoader):

```sh
openFPGALoader -b tangnano20k -f mackerel-f-@STAMP@.fs   # -f = persist to SPI flash
```

## Verify

```sh
sha256sum -c SHA256SUMS
```
EOF
sed -i "s/@STAMP@/$STAMP/g" "$STAGE/README.md"

( cd "$STAGE" && sha256sum *.bin *.jed *.pof *.fs > SHA256SUMS )

echo
echo "==================================================================="
echo "  Mackerel-68k firmware+logic release $STAMP"
echo "  staging: $STAGE"
echo "  artifacts: $STAGED staged (all built cleanly)"
echo "==================================================================="
