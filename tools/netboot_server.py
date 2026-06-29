#!/usr/bin/env python3
"""Mackerel-F netboot server.

Streams a kernel image (and its ROMfs) to the board's bootloader `netboot` command
over raw TCP. The board's W5500 NIC runs the TCP/IP stack in hardware, so the
bootloader just opens a socket and reads; this server sends the files and closes.

Wire format (one connection):

    [u32 big-endian length][IMAGE bytes][u32 big-endian length][ROMFS bytes]

then the server closes the connection. The bootloader loads IMAGE to PROGRAM_START
(0x400) and ROMFS to the ROMfs region (0x7A0000), then jumps to the kernel.

The files are re-read on every connection, so you can rebuild between netboots
without restarting the server. It loops until Ctrl-C.

Network setup: the bootloader's `netboot` command (firmware/bootloader.c,
handler_netboot) uses a *static* configuration -- by default the board is
192.168.1.199 and it connects to a server at 192.168.1.200:5000. Run this server on
that machine/port (or change the addresses in both places to match your LAN), with
the board on the same subnet and an Ethernet link up.

Usage:
    ./netboot_server.py [IMAGE] [ROMFS] [-p PORT] [--host ADDR]

IMAGE/ROMFS default to ./image.bin and ./romf.bin, so the simplest workflow is to
run it from the kernel build tree (e.g. ~/Workspace/mackerel-linux):

    cd ~/Workspace/mackerel-linux && /path/to/tools/netboot_server.py
"""
import argparse
import socket
import struct
import sys


def length_prefixed(path):
    """Read a file and return struct {u32 BE length}{bytes}."""
    with open(path, "rb") as f:
        data = f.read()
    return struct.pack(">I", len(data)) + data


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("image", nargs="?", default="image.bin",
                    help="kernel flat binary, loaded to 0x400 (default: ./image.bin)")
    ap.add_argument("romfs", nargs="?", default="romf.bin",
                    help="ROMfs image, loaded to 0x7A0000 (default: ./romf.bin)")
    ap.add_argument("-p", "--port", type=int, default=5000,
                    help="TCP port to listen on (default: 5000)")
    ap.add_argument("--host", default="0.0.0.0",
                    help="bind address (default: 0.0.0.0, all interfaces)")
    args = ap.parse_args()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        srv.bind((args.host, args.port))
    except OSError as e:
        sys.exit(f"netboot: cannot bind {args.host}:{args.port}: {e}")
    srv.listen(1)
    print(f"netboot: serving {args.image} (+ {args.romfs} if present) on "
          f"{args.host}:{args.port} -- run `netboot` on the board (Ctrl-C to stop)", flush=True)

    try:
        while True:
            conn, addr = srv.accept()
            try:
                # Re-read each time so a rebuild is picked up without a restart.
                payload = length_prefixed(args.image)
            except OSError as e:
                print(f"netboot: [{addr[0]}] cannot read {args.image}: {e}", flush=True)
                conn.close()
                continue
            # ROMfs is optional
            try:
                payload += length_prefixed(args.romfs)
            except FileNotFoundError:
                print(f"netboot: [{addr[0]}] no {args.romfs}, sending image only", flush=True)
            print(f"netboot: [{addr[0]}] sending {len(payload)} bytes...", flush=True)
            try:
                conn.sendall(payload)
                print(f"netboot: [{addr[0]}] sent OK", flush=True)
            except OSError as e:
                print(f"netboot: [{addr[0]}] send failed: {e}", flush=True)
            finally:
                conn.close()
    except KeyboardInterrupt:
        print("\nnetboot: stopped")


if __name__ == "__main__":
    main()
