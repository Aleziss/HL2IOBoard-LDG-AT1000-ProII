# HL2IOBoard-LDG-AT1000-ProII

LDG AT-1000 Pro II antenna tuner support for Hermes Lite 2 IO Board

## Description

This firmware adds LDG AT-1000 Pro II automatic antenna tuner support to the 
[Hermes Lite 2 IO Board](https://github.com/jimahlstrom/HL2IOBoard) designed by Jim Ahlstrom N2ADR.
It also includes amplifier control on J6 Out5 to protect the amplifier during tuning.

This code is based on the original n2adr_basic firmware by Jim Ahlstrom N2ADR.
Only `main.c` and `icom_ah4.c` have been modified.

## Important Notes

- The LDG AT-1000 Pro II timing behavior differs from the standard Icom AH-4 protocol
- Tested with Thetis v2.10.3.12 for Hermes Lite 2 by MI0BOT (Reid)
- The HL2 alone (5W max) may not provide enough power for the LDG (requires 10-15 watts)
- An external amplifier may be required

## Hardware Requirements

- Hermes Lite 2 with IO Board
- LDG AT-1000 Pro II (or compatible)
- 4.7K ohm resistor for KEY line pull-up
- External 12V power supply for the LDG (DO NOT use IO Board 12V switched line)

## Wiring

| LDG Cable | IO Board |
|-----------|----------|
| START (ring) | J6 pin 6 (GPIO22_Out6 low-side switch) |
| KEY (tip) | J8 pin 2 (GPIO18_In2) + 4.7K ohm pull-up to 3.3V (3V2 on IO board) |
| GND (sleeve) | GND (G1 or G2 on IO Board) |
| POWER (+12V) | External 12V power supply only - DO NOT use IO Board switched 12V line |

⚠️ **WARNING: Inductive loads on J6 Out5**
If your amplifier uses relay coils on the KEY/PTT line, you MUST place a flyback diode 
across the relay coil to protect the IO Board low-side switch (TBD62381).
A 1N4148 or 1N4001 diode across the relay coil is sufficient.
See [IO Board documentation](https://github.com/jimahlstrom/HL2IOBoard) for details.

⚠️ **WARNING: DO NOT connect START to J4 pin 6** — as per the original code instructed - 
J4 is an active 5V output that will block the LDG tuner. Use J6 pin 6 (low-side switch) only.

⚠️ **WARNING: DO NOT power the LDG tuner from the IO Board 12V switched line** — use an 
external 12V power supply only.

## How It Works

The LDG AT-1000 Pro II behaves differently from the standard Icom AH-4 protocol:

1. Thetis sends tune request via CTRL+TUNE
2. Pico asserts START high for 500ms then releases
3. LDG pulls KEY low **after** START is released (unlike AH-4 which pulls KEY low while START is high)
4. Pico requests RF from Thetis (0xEE) - requires 10-15 watts
5. LDG tunes the antenna and pulls KEY high when complete
6. Pico stops RF transmission

## Important Behavior Notes

### Tuning Success vs Error Detection
The LDG AT-1000 Pro II does not send an error code on the KEY line when tuning fails.
The IO Board cannot distinguish between a successful tune and a failed tune - in both cases
KEY returns high and the IO Board stops RF transmission (returns 0x00).
If tuning fails (insufficient RF, antenna too far out of range, etc.), the LDG will simply
abort and return to its previous state without any error indication to the IO Board.

### Thetis Power Settings (v2.10.3.12)
⚠️ **"Use Tune Slider"** does not work correctly with CTRL+TUNE in Thetis v2.10.3.12 —
it uses full DRIVE power (0dB) instead of the tune slider level.

✅ **Recommended: Use "Use Fixed Drive"** in Setup → Transmit → Tune and set a fixed 
drive level to achieve 10-15 watts output for reliable LDG tuning.

## Amplifier Control

Out5 (J6 pin 5) controls an external amplifier:
- **TX normal** → Out5 HIGH (amplifier active)
- **RX** → Out5 LOW (amplifier bypassed)
- **During tuning** → Out5 LOW (amplifier bypassed - LDG requires 10-15 watts for tuning, excessive power could damage the tuner or prevent successful tuning)

## Installation

1. Download and install the [Hermes Lite 2 IO Board](https://github.com/jimahlstrom/HL2IOBoard) 
   project from Jim N2ADR
2. Replace `n2adr_basic/main.c` with the `main.c` from this repository
3. Replace `n2adr_lib/icom_ah4.c` with the `icom_ah4.c` from this repository
4. Recompile following Jim's instructions, or use the provided `main.uf2` directly

## Quick Install (no compilation required)

1. Power off the HL2
2. Hold BOOTSEL button on Pico and connect USB cable to PC
3. Copy `main.uf2` to the Pico drive
4. Disconnect USB and power on the HL2

## Thetis Settings

- Use **CTRL+TUNE** to start automatic tuning
- For power level, use **Setup → Transmit → Tune → Use Fixed Drive**
- Set fixed drive to achieve 10-15 watts output
- Note: "Use Tune Slider" with CTRL+TUNE may use full drive power (known issue)

## Error Codes

| Code | Description |
|------|-------------|
| 0x00 | Success |
| 0xFA | KEY not high at start - check wiring and pull-up resistor |
| 0xFB | Timeout - KEY never went low after START released |
| 0xFD | Safety timeout - tuning exceeded 15 seconds |

## Example Setup (VA2CST)

This firmware was developed and tested with the following station setup:

HL2 (5W) → Pre-drive amplifier (~60W) → Final tube amplifier (~700W) → LDG AT-1000 Pro II → Antenna

**Signal flow control:**
- **RX** → J6 Out5 floating = both amplifiers in receive mode
- **TX normal (MOX)** → J6 Out5 grounded = both amplifiers in transmit mode, this permit full output power for normal operation
- **Tune mode (TUNE)** → J6 Out5 grounded = both amplifiers in transmit mode, this allow to peak final amplifier at lower power
- **Automatic Tuning (CTRL+TUNE)** → Out5 floating = final amplifier bypassed, pre-drive only (10-15W for LDG tuning)

The pre-drive amplifier is controlled directly by the HL2 RCA PTT jack.
The final tube amplifier is controlled by J6 Out5 on the IO Board.

This setup ensures the LDG receives only 10-15 watts during automatic tuning,
protecting both the LDG tuner and the final amplifier from high SWR conditions.

## Known Limitations

### Thetis RF Timeout (v2.10.3.12)
Thetis seems to stop RF transmission after approximately 7-8 seconds during CTRL+TUNE,
regardless of the tuning state. This may prevent successful tuning if the LDG
requires more time to complete the tuning sequence.

**Workaround:** None currently available - waiting for Thetis fix.
**Status:** Reported to MI0BOT (Reid) - Issue #127
https://github.com/mi0bot/OpenHPSDR-Thetis/issues/127

## Credits

- Original IO Board firmware by Jim Ahlstrom N2ADR - https://github.com/jimahlstrom/HL2IOBoard
- Thetis for Hermes Lite 2 by Reid MI0BOT - https://github.com/mi0bot/OpenHPSDR-Thetis
- LDG AT-1000 Pro II modifications by Claude Perreault VA2CST - 2026

## License

MIT License - see original project for details
