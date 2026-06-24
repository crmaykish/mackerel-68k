# Mackerel-30 Clock & Reset Supervisor — Design Context

> Drop-in context for KiCad + Claude Code. Rename to `CLAUDE.md` (or merge into an
> existing one) so Claude Code auto-loads it, or point Claude Code at this file when
> asking it to review the schematic/PCB. The goal of this doc is to let an automated
> reviewer check the actual `.kicad_sch` / `.kicad_pcb` files against the *intended*
> design and the hard-won gotchas below.

## Purpose

A small clock-generation + reset-supervisor circuit for the **Mackerel-30** (Motorola
68030-based SBC, 5 V logic). It replaces:

- **Three discrete oscillator cans** — 50 MHz DRAM controller clock, 24 MHz CPU clock,
  3.6864 MHz DUART clock — with one programmable **Si5351A** synthesizer.
- **The DS1233 reset chip** — reset is now sequenced in firmware by an **ATtiny1614**
  supervisor (with the AVR's internal brown-out detector covering the AVR itself).

The supervisor programs the Si5351 over I²C at power-on, then sequences the 68030's
`/RESET` and `/HALT` lines. It also exposes a manual reset button, an optional status
LED, and a spare-GPIO expansion header.

## Architecture / block summary

| Ref | Part | Role | Supply |
|-----|------|------|--------|
| U1 | ATtiny1614 (SOIC-14) | Supervisor: programs Si5351 (TWI), drives /RESET + /HALT, button, LED | **3.3 V** |
| Y1 | Si5351A-B-GT (MSOP-10) | 3-output clock synth, I²C addr 0x60, XTAL reference | **3.3 V** |
| Y2 | 25.000 MHz, 8 pF, 3.2×2.5 4-pad crystal | Si5351 reference | — |
| U3 | 74AHCT125 (SOIC-14) | Push-pull buffer: 3.3 V clocks → **5 V** (3 gates + 1 spare) | **5 V** |
| U4 | 74LVC07A (SOIC-14) | Open-drain buffer: 3.3 V /RESET,/HALT → 5 V reset net (2 gates + 4 spare) | **3.3 V** |
| U2 | MCP1700-3302E/TO (TO-92) | 5 V → 3.3 V LDO for the logic island | in 5 V / out 3.3 V |

**Power domains**
- **+5 V**: board input. Powers U3 (AHCT), the /RESET+/HALT pull-ups, and the MCP1700 input.
- **+3.3 V**: generated locally by U2 (MCP1700). Powers U1 (AVR), Y1 (Si5351), U4 (LVC07).
- The only 5 V loads downstream are the legacy 68k chips, reached through the AHCT/LVC buffers.

## Frequency plan (firmware intent — informs register config, not netlist)

- 25 MHz crystal → two on-chip PLLs.
- **DUART = 3.6864 MHz exact**, isolated on **PLL_A** (VCO 884.736 MHz; fractional
  multiply; MultiSynth 240). Fractional jitter is irrelevant at UART speed; exactness
  is what matters for baud accuracy. Accuracy budget = crystal ppm only.
- **DRAM = 50 MHz and CPU = 24 MHz** share **PLL_B**: VCO 600 MHz (integer ×24),
  MultiSynth 12 → 50 MHz, MultiSynth 25 → 24 MHz. Both integer-mode (lowest jitter).
- **Experimentation mode**: set PLL_B VCO = 900 MHz (×36). Then CPU 18–40 MHz and
  DRAM 40–66 MHz are all reachable on the per-output MultiSynth dividers alone — no PLL
  retune, so no relock and no lock-loss glitch when sweeping. Bracket any live change
  with /RESET + /HALT.
- **Crystal load cap = 8 pF** → Si5351 **register 183 (0xB7) bits [7:6] = 0b10**.
  This MUST be set in firmware; the power-on default is 10 pF and would pull this 8 pF
  part off-frequency.
- Output→pin assignment is route-driven (free), as long as each output's PLL source is
  set correctly in firmware: DUART from PLL_A; DRAM + CPU from PLL_B.

## Pin maps

### U1 — ATtiny1614 (SOIC-14)
| Pin | Port | Net / function |
|-----|------|----------------|
| 1 | VDD | +3.3 V |
| 2 | PA4 | → U4A input (/RESET drive) |
| 3 | PA5 | → U4B input (/HALT drive) |
| 4 | PA6 | → R9 (1k) → LED D1 → GND (status, optional) |
| 5 | PA7 | reset button SW1 → GND (internal pull-up in firmware) |
| 6 | PB3 | spare → J2 |
| 7 | PB2 | spare → J2 (also USART TXD) |
| 8 | PB1 | **SDA** → Si5351 pin 5 |
| 9 | PB0 | **SCL** → Si5351 pin 4 |
| 10 | PA0 | UPDI → R1 (4.7k series) → J1 |
| 11 | PA1 | spare → J2 (also alt-SDA) |
| 12 | PA2 | spare → J2 (also alt-SCL) |
| 13 | PA3 | spare → J2 (also EXTCLK in) |
| 14 | GND | GND |

TWI is at the **default** PORTMUX position (SDA = PB1, SCL = PB0). Alternate position
(PA1/PA2) is intentionally left free on the header.

### Y1 — Si5351A-B-GT (MSOP-10)
| Pin | Name | Net |
|-----|------|-----|
| 1 | VDD | +3.3 V |
| 2 | XA | crystal Y2 |
| 3 | XB | crystal Y2 |
| 4 | SCL | AVR PB0 |
| 5 | SDA | AVR PB1 |
| 6 | CLK2 | → U3C → R4 → CLK_DUART |
| 7 | VDDO | +3.3 V (tied to VDD) |
| 8 | GND | GND |
| 9 | CLK1 | → U3B → R3 → CLK_CPU |
| 10 | CLK0 | → U3A → R2 → CLK_DRAM |

## Component list (this subcircuit)

- **U1** ATtiny1614-SSFR (SOIC-14). Decoupling: **C6** 0.1 µF on 3.3 V.
- **U2** MCP1700-3302E/TO (TO-92, 3.3 V out). **C4** 1 µF (out), **C5** 1 µF (in).
- **U3** 74AHCT125 (SOIC-14; e.g. Diodes 74AHCT125S14-13). **VCC = +5 V**. **C3** 0.1 µF.
  U3A/B/C = clock buffers; U3D = spare (tied off); U3E = power/decoupling unit.
- **U4** 74LVC07A (SOIC-14; e.g. Nexperia 74LVC07AD). **VCC = +3.3 V**. **C7** 0.1 µF.
  U4A/B = /RESET, /HALT buffers; U4C–F = spare (tied off).
- **Y1** Si5351A-B-GT (MSOP-10). **C1**, **C2** 0.1 µF (separate caps on VDD and VDDO).
- **Y2** 25.000 MHz, **CL 8 pF**, 3.2×2.5 mm 4-pad fundamental crystal
  (e.g. Abracon FC3BSEEDP25.0-T3). 6/8/10 pF acceptable if the firmware load-cap reg matches.
- **R2, R3, R4** 33 Ω — clock series termination (at AHCT outputs).
- **R5, R6** 4.7 kΩ — I²C pull-ups → **+3.3 V**.
- **R7, R8** 4.7 kΩ — /RESET, /HALT pull-ups → **+5 V**.
- **R1** 4.7 kΩ — UPDI **series** resistor (not a pull-up).
- **R9** 1 kΩ — LED limit.
- **SW1** tactile — manual reset → PA7.
- **D1** LED — status (optional / DNP-able).
- **J1** Conn_01x03 — UPDI header (VCC / UPDI / GND). Keep separate from J2.
- **J2** Conn_01x07 — expansion header: 5 spare GPIOs (PB2, PB3, PA1, PA2, PA3) + 3V3 + GND.

## Critical design rules (the gotchas — verify these first)

1. **U4 (LVC07) VCC = +3.3 V, NOT +5 V.** At VCC = 5 V the LVC07 input threshold is
   VIH = 0.7×VCC = 3.5 V, which the AVR's 3.3 V drive does **not** clear → broken reset
   path. At VCC = 3.3 V, VIH = 2.0 V and 3.3 V clears it. The open-drain *outputs* are
   pulled to **+5 V** by R7/R8 — that's the 5 V-tolerant part and is correct. Only the
   chip supply is 3.3 V. (This was an actual bug found and fixed; C7 decouples on 3.3 V.)
2. **U3 (AHCT) VCC = +5 V.** It translates the clocks *up* to 5 V. Must be **AHCT/HCT**
   (TTL threshold ~2.0 V, accepts 3.3 V input), never plain AHC (CMOS threshold).
3. **/RESET and /HALT are open-drain, active-low, wired-OR.** Driven low by the LVC07,
   by the 68030 itself (RESET instruction / double-fault on /HALT), and by any manual
   reset. Nothing may push-pull-drive these nets. AVR side is push-pull GPIO into the
   LVC07 input; the LVC07 provides the open-drain output.
4. **Pull-up rails are different and easy to transpose** (all are 4.7 kΩ):
   I²C R5/R6 → **+3.3 V**; reset/halt R7/R8 → **+5 V**.
5. **I²C has no crossover** — SDA↔SDA, SCL↔SCL. AVR PB0 (SCL) ↔ Si5351 pin 4 (SCL);
   AVR PB1 (SDA) ↔ Si5351 pin 5 (SDA).
6. **Crystal Y2**: pins 1 & 3 = resonator terminals → XA/XB (symmetric, interchangeable);
   pins 2 & 4 = case ground → GND (both). No external load caps (Si5351 internal caps).
7. **Si5351 VDD (1) and VDDO (7) both on +3.3 V**, each with its own decoupling cap
   (C1 on VDD, C2 on VDDO). Tying them satisfies the VDDO-up-with-VDD sequencing rule.
8. **Unused gates tied off**: U3D (AHCT) input→GND, /OE→+5 V, output NC.
   U4C–F (LVC07) input→GND, output NC (no /OE on a 7407-style buffer).
9. **UPDI**: R1 is a **series** resistor into PA0, not a pull-up. Keep PA0 as UPDI; do
   not reclaim it as RESET/GPIO via fuse (that loses UPDI without an HV programmer).
10. **PWR_FLAG** required on +5 V and GND (consumed/referenced off-sheet). +3.3 V is
    driven by the LDO output, but flag it too if ERC complains.

## Firmware notes (for the supervisor, not the netlist)

- Si5351 I²C address **0x60**.
- **Set reg 0xB7 = 0b10 (8 pF)** before anything else — matches Y2; default is wrong.
- Enable the AVR **brown-out detector** in fuses (covers the AVR's own power supervision).
- **Park spare GPIOs** in init (PB2/PB3/PA1/PA2/PA3): drive output-low or enable pull-ups,
  so floating inputs on the J2 header don't draw crowbar current / pick up noise.
- Reset button PA7: `PORTA.PIN7CTRL = PORT_PULLUPEN_bm`, debounce in firmware (no external RC).
- **Boot sequence**: power-good → AVR boots → program Si5351 → wait for PLL lock →
  assert /RESET + /HALT low → hold ~100 ms → release.
- **Glitch-free runtime speed change**: assert /RESET+/HALT, rewrite only the output
  MultiSynth dividers (PLL_B VCO stays fixed at 900 MHz → no relock), release.
- **Lock LED**: Si5351 has no lock pin; poll the loss-of-lock status bit over I²C and
  reflect it on PA6.

## Layout guidance (for the PCB pass)

Fuss priority, highest first:
1. **Crystal hard against the Si5351** — XA/XB traces as short as possible, away from the
   clock outputs; both case-ground pads to the plane. This beats everything for "keep short."
2. **Si5351 → AHCT125 short** — this hop is unterminated 3.3 V.
3. **33 Ω series resistors at the AHCT output pins** (source termination — must be at the
   source, not near the load).
4. **Inline probe pads** on the load side of each series R — pass-through, NOT a stub —
   each with an adjacent GND via for a short scope ground.
5. **50 MHz DRAM line shortest**, then CPU, then DUART (DUART barely cares). All clocks
   over **continuous, unbroken ground** — no plane splits/voids under a clock trace.
6. **Decoupling tight to each IC power pin**: C1 at Si5351 VDD, C2 at VDDO, C3 at AHCT,
   C7 at LVC07, C6 at AVR, C4/C5 at the LDO.
7. CPU clock may **daisy-chain** CPU + CPLD (inline, far load at the trace end) — they
   share one clock for synchronous bus glue.
8. **J2 header**: silkscreen each pin's function and a **"3V3 only"** warning (these pins
   are not 5 V-tolerant — the AVR runs at 3.3 V).

## Review checklist for Claude Code (run against the actual files)

Schematic (`.kicad_sch`):
- [ ] U4 (LVC07) VCC net is **+3.3 V** on all units; C7 on +3.3 V; value reads 74LVC07A.
- [ ] U3 (AHCT) VCC net is **+5 V**; C3 on +5 V; part is AHCT (not AHC).
- [ ] R5/R6 pull to **+3.3 V**; R7/R8 pull to **+5 V** (not transposed).
- [ ] SCL: AVR PB0 (pin 9) ↔ Si5351 SCL (pin 4). SDA: AVR PB1 (pin 8) ↔ Si5351 SDA (pin 5). No cross.
- [ ] /RESET and /HALT nets: LVC07 open-drain outputs + R7/R8 pull-ups to 5 V; exported as labels.
- [ ] Si5351 VDD (1) and VDDO (7) both on +3.3 V; C1 and C2 both present.
- [ ] Crystal Y2: pins 2 & 4 → GND, pins 1 & 3 → XA/XB.
- [ ] J2 carries exactly PB2, PB3, PA1, PA2, PA3 + 3V3 + GND. No used pins
      (PB0/PB1/PA4/PA5/PA6/PA7) accidentally on J2.
- [ ] Unused gates tied off: U3D (in→GND, /OE→+5 V, out NC); U4C–F (in→GND, out NC).
- [ ] UPDI R1 is in **series** into PA0 (not a pull-up). PA0 = UPDI only.
- [ ] Every IC has a local decoupling cap.
- [ ] PWR_FLAG present on +5 V and GND. ERC clean.

PCB (`.kicad_pcb`), if present:
- [ ] 33 Ω series resistors placed at the AHCT output pins (source side).
- [ ] Crystal placed immediately adjacent to the Si5351; XA/XB short; case pads grounded.
- [ ] Clock traces over continuous ground (no plane split/void underneath); 50 MHz shortest.
- [ ] Inline probe pads (pass-through, not stubs) with adjacent GND via on each clock.
- [ ] Decoupling caps within a few mm of their IC power pins.
- [ ] Run DRC. (Note: a static review can't predict 50 MHz signal integrity — that's what
      the probe points are for after fab.)

## History / provenance

Designed as a clock + reset daughter circuit for the Mackerel-30. Replaces 3 oscillator
cans + DS1233 with Si5351 + ATtiny1614. Schematic complete and ERC-clean as of this doc;
next step is PCB layout following the guidance above.
