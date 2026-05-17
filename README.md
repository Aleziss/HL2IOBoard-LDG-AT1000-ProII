# HL2IOBoard-LDG-AT1000-ProII

LDG AT-1000 Pro II antenna tuner support for Hermes Lite 2 IO Board

## Table of Contents
- [Description](#description)
- TUNER Section
   - [Important Notes](#important-notes)
   - [Hardware Requirements](#hardware-requirements)
   - [Wiring](#wiring-tuner-and-amplifier)
   - [How It Works](#how-it-works)
   - [Amplifier Control](#amplifier-control)
   - [Installation](#installation)
   - [Quick Install](#quick-install-no-compilation-required)
   - [Thetis Operation and Settings](#thetis-operation-and-settings)
   - [Error Codes](#error-codes)
   - [Known Limitations](#known-limitations)
- BCD Band Decoder Section
   - [BCD Output Logic Table](#bcd-output-logic-table)
   - [Wiring](#wiring-bcd)
- ALT RX Section
   - [RF Input Modes Explained](#rf-input-modes-explained)
   - [RF Input Mode Switch](#rf-input-mode-switch)
   - [Wiring](#wiring-rf-input)
- [Credits](#credits)

## Description

This firmware adds support for the **LDG AT-1000 Pro II** automatic antenna tuner to the [Hermes Lite 2 IO Board](https://github.com/jimahlstrom/HL2IOBoard) designed by Jim Ahlstrom, N2ADR.
It enables control between the Hermes Lite 2 and the tuner radio port for automated tuning cycles.

**Key Features:**
* **LDG Integration:** Control of the LDG AT-1000 Pro II via a modified Icom AH-4 protocol (Requires tuner firmware V1.7).
   * Altough not tested, there are good chance that this code will also work with AT-100, 200 and 600 Pro II
* **Amplifier Control:** Integrated interlock logic on J6 Out5 to protect both the tuner and the amplifier during tuning.
* **Band Voltage:** Supports Yaesu FT-817 analog band voltage on J4 Out8.
* **BCD Band Decoder:** 4-bit Yaesu-standard BCD output on J6/J4 Out1-4.
* **RF Input Mode Control:** Option for a dedicated receive antenna on ALT RX SMA connector J9 with adaptive predistortion sampling on PURE SMA connector J10.

This code is based on the original `n2adr_basic` firmware by Jim Ahlstrom N2ADR.
BCD band decoding logic inspired by Dalton Williams (W5EIM) is used with original N2ADR logic control.
Only `main.c` and `icom_ah4.c` have been modified.

## 🆕 Update (April 26th 2026): RF Input Mode Control
This new feature allows you to install a SPDT switch on J8 In5 to select between
* normal HL2 RX on ANT input with Pure Signal feedback on J10 (Mode 0) or
* dedicated receive antenna on ALT RX J9 with Pure Signal feedback on J10 (Mode 2).

You can [view information here](#rf-input-mode-switch).

## Update (April 22nd 2026): BCD Band Decoder 
This feature allows the IO Board to drive
* external Low Pass Filters (LPF) for Solid State Power Amplifiers (SSPA)
* band change control on amplifiers
* automatic antenna switches based on band changes.
* **Standard Yaesu BCD Output:** Outputs 4-bit band data on **J6/J4 Out1-4**.

You can [view table information](#bcd-output-logic-table).

---
## 🔧 LDG AT-1000 Pro II Tuner

### Important Notes

- The LDG AT-1000 Pro II timing behavior differs from the standard Icom AH-4 protocol
- Tested with Thetis v2.10.3.12, .13 and .14 for Hermes Lite 2 by MI0BOT (Reid), known issues documented below
- The HL2 alone (5W max) won't provide enough power for the LDG AT-1000 PRO II 
- An external amplifier is required for the AT-1000 PRO II, 10-25 watts for effective tuning
- AT-1000 PRO II with firmware V1.8 has disabled the METER and RADIO interface ports for safety reasons 
so this code won't work with your tuner if it uses that version.

### Hardware Requirements

- Hermes Lite 2 with IO Board
- LDG AT-1000 Pro II (firmware V1.7 or compatible)
- Wire to solder from J8 pin 2 to P3 pull-up (4.7k 5V)
- External 12V power supply for the LDG (DO NOT use IO Board 12V switched line)

### Wiring Tuner and Amplifier

| Signal | IO Board Pin | Function |
|-----------|----------|----------|
| **START (Tuner)** | J6 pin 6 (GPIO22_Out6) | Start line to LDG (low-side switch) |
| **KEY (Tuner)** | J8 pin 2 (GPIO18_In2) | Key line from LDG (4.7K pull-up to P3) |
| **Amplifier KEY** | J6 pin 5 (GPIO10_Out5) | Amplifier Interlock (Out5) |
| **GND** | G1, G2 or G3 | Shared Ground |

⚠️ **WARNING: Inductive loads on J6 Out5**
If your amplifier uses relay coils on the KEY/PTT line, you MUST place a flyback diode 
across the relay coil to protect the IO Board low-side switch (TBD62381).
A 1N4148 or 1N4001 diode across the relay coil is sufficient.
See [IO Board documentation](https://github.com/jimahlstrom/HL2IOBoard#design-of-the-io-board-hardware) for details.

⚠️ **WARNING: DO NOT connect START to J4 pin 6** — as per the original code instructed, 
J4 is an active 5V output that will block the LDG tuner. Use J6 pin 6 (low-side switch) only.

⚠️ **WARNING: DO NOT power the LDG tuner from the IO Board 12V switched line** — use an 
external 12V power supply only.

### How It Works

The LDG AT-1000 Pro II behaves differently from the standard Icom AH-4 protocol:

1. Thetis sends tune request via CTRL+TUN
2. Pico asserts START high for 600ms then releases
3. LDG pulls KEY low **after** START is released (unlike AH-4 which pulls KEY low while START is high)
4. Pico requests RF from Thetis (0xEE) - requires 10-25 watts
5. LDG tunes the antenna and pulls KEY high when complete
6. Pico stops RF transmission

## Important Behavior Notes

### Tuning Success vs Error Detection
The LDG AT-1000 Pro II does not send an error code on the KEY line when tuning fails.
The IO Board cannot distinguish between a successful tune and a failed tune - in both cases
KEY returns high and the IO Board stops RF transmission (returns 0x00).
If tuning fails (insufficient RF, antenna too far out of range, etc.), the LDG will simply
abort and return to its previous state without any error indication to the IO Board.

### Thetis Power Settings
Set value of "Use Fixed Drive" to achieve 10-25 watts output with CTRL+TUN for reliable LDG AT-1000 Pro II tuning in Setup → Transmit.

⚠️ v2.10.3.13/.14: If "Use Tune Slider" is selected while CTRL+TUN, it will now use "Use Fixed Drive" value instead of full power.
   - If you use TUN only, "Use Tune Slider" level will be use.   

❌ v2.10.3.12 : If "Use Tune Slider" is selected with CTRL+TUN, **it will use full drive power (0dB)**.

✅ Recommended: Select and use "Use Fixed Drive" value only. 

### Amplifier Control

Out5 (J6 pin 5) controls an external amplifier:
- **Amplifier Interlock:** Automatically bypasses your amplifier during LDG tuning.
- **Safety Feature:** The amplifier remains bypassed after tuning until HL2 returns to RX mode once. This prevents "hot-switching" and protects your tuner's relays.
- **TX normal** → Out5 HIGH (amplifier active)
- **RX** → Out5 LOW (amplifier bypassed)
- **During tuning** → Out5 LOW (amplifier bypassed)

### Installation

1. Download and install the [Hermes Lite 2 IO Board](https://github.com/jimahlstrom/HL2IOBoard) 
   project from Jim N2ADR
2. Replace `n2adr_basic/main.c` with the `main.c` from this repository
3. Replace `n2adr_lib/icom_ah4.c` with the `icom_ah4.c` from this repository
4. Recompile the project and uplaod `main.uf2` from `n2adr_basic/build` to the Pico drive

### Quick Install (no compilation required)

1. Power off the HL2
2. Hold BOOTSEL button on Pico and connect USB cable to PC
3. Copy `main.uf2` from this repository to the Pico drive
4. Disconnect USB and power on the HL2

### Thetis Operation and Settings

- Use **CTRL+TUN** to start automatic tuning
- For power level, use **Setup → Transmit → Tune → Use Fixed Drive**
- Set fixed drive to achieve 10-25 watts output
- Note: "Use Tune Slider" with CTRL+TUN may use full drive power (known issue)

### Error Codes

| Code | Description |
|------|-------------|
| 0x00 | Success |
| 0xFA | KEY not high at start - check wiring and pull-up resistor |
| 0xFB | Timeout - KEY never went low after START released |
| 0xFD | Safety timeout - tuning exceeded 15 seconds |

### Example Setup (VA2CST)

This firmware was developed and tested with the following station setup:

HL2 (5W) → Pre-drive amplifier (~60W) → Final tube amplifier (~700W) → LDG AT-1000 Pro II → Antenna

The pre-drive amplifier is controlled directly by the HL2 EXTTR RCA jack.
The final tube amplifier is controlled by J6 Out5 on the IO Board.

This setup ensures the LDG receives only 10-25 watts during automatic tuning,
protecting both the LDG tuner and the final amplifier from high SWR or over power conditions.

**Signal flow control:**
- **RX** → J6 Out5 floating = both amplifiers in receive mode
- **TX normal (MOX)** → J6 Out5 grounded = both amplifiers in transmit mode, this allows full output power for normal operation
- **Tune mode (TUN)** → J6 Out5 grounded = both amplifiers in transmit mode, this allows to peak final amplifier at lower power
- **Automatic Tuning (CTRL+TUN)** → Out5 floating = final amplifier bypassed, pre-drive only (10-25W for LDG tuning)

### IO board wiring example (VA2CST)
![](./assets/20260424_015018.jpg)

Using 2.54mm male and female pin headers,
- Yellow wire - J8 pin 2 (KEY line) with pull up resistor to 4.7k 5V (P3) → J7 pin 2 (DB9)
- Orange wire - J6 pin 6 (START line) → J7 pin 3 (DB9)
- Red wire - J6 pin 5 (amplifier KEY) → J7 pin 7 (DB9)
- Black wire - G1 (GND) → J7 pin 5 (DB9), GND shared for tuner and amplifier on DB9 connector

For J7 (DB9) you can chose whatever pin number order you like for your project.
On my end, I kept the RS232 pins numbers (even though it is NOT a serial communication port) being:
- pin2 KEY line (RXD receiving from the LDG tuner)
- pin3 START line (TXD sending to the LDG tuner)
- pin5 GND
- pin7 Amplifier KEY (RTS Ready To Send to the second amp)

### Pull-Up resistor connection
**Note:** The IO Board already has 4.7K ohm pull-up resistors on P3 and P4 (to +5V). 
Simply connect P3 or P4 to J8 pin 2 with a short wire instead of adding an external resistor.

![](./assets/P3_PU_J8_2.png)

![](./assets/20260424_014728.jpg)

### YouTube Video Demonstration

You can see [HERE](https://youtu.be/ttHCVzRcAcU) a crude video demonstration of the system working.

### Known Limitations

### Thetis-HL2 RF Timeout (v2.10.3.12, .13 and .14)
Thetis seems to stop RF transmission after approximately 7-8 seconds during CTRL+TUN,
regardless of the tuning state. This may prevent successful tuning if the LDG
requires more time to complete the tuning sequence.

**Workaround:** For automatic tuning >7s, none currently available - waiting for Thetis fix.

**Status:** Reported to MI0BOT (Reid) - Issue #127
https://github.com/mi0bot/OpenHPSDR-Thetis/issues/127

**Manual Workaround:** For tuning requiring >7s
1. Manually bypass final amplifier connected on J6 Out5
2. Hit TUN on Thetis, this should provide 10-25W for unlimited time
3. Hold "TUNE" on LDG tuner between >500ms and <2.5s, tuner will start up to 15s
4. Once tune achieve, turn off TUN on Thetis software and reengage final amplifier on J6 Out5

### LDG AT-1000 Pro II Firmware Compatibility

⚠️ **Important:** This code requires LDG firmware version 1.7.
LDG firmware version 1.8 disabled the METER and RADIO interface ports for safety reasons,
but LDG has since released firmware V1.7 which restores these ports.

**Current firmware is V1.7** - if you have V1.8 installed, contact LDG to downgrade 
to V1.7 to restore radio port functionality.

---
## 📡 BCD Band Decoder

### BCD Output Logic Table

This table shows the state of **Out 1-4** (available on both J4 & J6) for each band.

| Band | BCD Code (DCBA) | Out 1 (D) | Out 2 (C) | Out 3 (B) | Out 4 (A) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **160m** | 0001 | 0 | 0 | 0 | **1** |
| **80m** | 0010 | 0 | 0 | **1** | 0 |
| **40m/60m**| 0011 | 0 | 0 | **1** | **1** |
| **30m** | 0100 | 0 | **1** | 0 | 0 |
| **20m** | 0101 | 0 | **1** | 0 | **1** |
| **17m** | 0110 | 0 | **1** | **1** | 0 |
| **15m** | 0111 | 0 | **1** | **1** | **1** |
| **12m** | 1000 | **1** | 0 | 0 | 0 |
| **10m** | 1001 | **1** | 0 | 0 | **1** |
| **6m** (not on HL2) | 1010 | **1** | 0 | **1** | 0 |

### Wiring BCD

| Signal | IO Board Pin | Function |
|-----------|----------|----------|
| **BCD Data A** | J4/J6 pin 4 (GPIO11_Out4) | Yaesu BCD Bit 0 (LSB) |
| **BCD Data B** | J4/J6 pin 3 (GPIO20_Out3) | Yaesu BCD Bit 1 |
| **BCD Data C** | J4/J6 pin 2 (GPIO19_Out2) | Yaesu BCD Bit 2 |
| **BCD Data D** | J4/J6 pin 1 (GPIO16_Out1) | Yaesu BCD Bit 3 (MSB) |
| **Band Voltage** | J4 pin 8 (GPIO26_Out8) | Yaesu FT-817 Analog Band Volts |
| **GND** | G1, G2 or G3 | Shared Ground |

### Pin Mapping Reference

| Signal | J4 (5V Logic) | J6 (Low-side Switch) | Pico GPIO |
| :--- | :--- | :--- | :--- |
| **Data D** | Pin 1 | Pin 1 | GPIO 16 |
| **Data C** | Pin 2 | Pin 2 | GPIO 19 |
| **Data B** | Pin 3 | Pin 3 | GPIO 20 |
| **Data A** | Pin 4 | Pin 4 | GPIO 11 |

> **Operation Logic:**
> * **Logic 1:** J4 outputs **+5V** / J6 switch is **Closed** (Path to GND).
> * **Logic 0:** J4 outputs **0V** / J6 switch is **Open**.

---
## 🔀 RF Input Mode Control (ALT RX)

### RF Input Mode Switch

A physical SPDT switch can be used to select between RF input
* mode 0 (normal HL2 RX on ANT SMA connector and
* mode 2 (ALT RX on J9 SMA).
* both mode with adaptive predistortion sampling on PURE SMA connector J10

### RF Input Modes Explained

#### Mode 0 - Normal HL2 RX (switch to GND)
This is the standard HL2 operating mode.
* The receive signal comes from the ANT SMA connector on the HL2.
* The PURE signal input on SMA connector (J10) is mixed with the receive signal for linearity correction feedback.
* The HL2 internal T/R relay K2 operates normally, switching between RX and TX automatically.

#### Mode 2 - ALT RX with Pure Signal (switch to P4)
This mode is designed for stations using a dedicated receive antenna.
* The receive signal comes from the ALT RX SMA connector (J9) on the IO Board instead of the HL2 ANT SMA connector.
* During TX, the PURE signal input on SMA connector (J10) captures a sample of the transmitted signal and passes it to the HL2 for linearity correction (IMD reduction).
* The HL2 internal T/R relay K2 is held in RX position permanently — this is by design and has been confirmed safe by Jim Ahlstrom N2ADR: the 5W TX signal is not passed to the HL2 RX chain, only negligible incidental pickup may occur.

### Wiring RF Input

| Signal | IO Board Pin | Function |
|-----------|----------|----------|
| **RF Input** | J8 pin 5 (GPIO06_In5)| Dedicated ALT RX antenna |
| **GND** | G1, G2 or G3 | Shared Ground |

- Switch common (pin 1) → J8 pin 5 (In5 / GPIO06)
- Switch position 1 (pin 3) → P4 (already has R31 4.7K pullup to +5V on the IO Board)
- Switch position 2 (pin 2) → GND

![](./assets/ALT_RX_mode_0-2.png)

### Operation
- **Switch to P4** → In5 sees +5V via R31 4.7K pullup → Mode 2 active
- **Switch to GND** → In5 sees 0V → Mode 0 active

### Notes
- The 4.7K pullup resistor R31 on P4 is already populated on the IO Board — no additional resistor required
- J8 inputs are protected and accept 3.3V to 20V logic levels (per N2ADR documentation)
- This implementation is fully independent of Thetis and the HL2
- Default state follows switch position at power-up — Mode 0 when switch is to GND, Mode 2 when switch is to P4

### Optional Wiring (via Thetis)
As an alternative to the physical switch, Out7 can be used to control the mode directly from Thetis.
- Connect J4 Out7 to J8 In5
- In Thetis, go to Setup → General → Options → HL2 Options panel, click o7 to switch between modes by checking "Pin Control"
   - o7 LOW (dark square) → 0V → Mode 0 → i5 LOW (dark square)
   - o7 HIGH (green square) → 5V → Mode 2 → i5 HIGH (green square)
- With this method, mode 0 is always active at power-up since all outputs default to LOW when the HL2 starts
- ⚠️ Use caution in Thetis — activating other outputs may interfere with
   - BCD band decoder (Out1-4)
   - amplifier control (Out5)
   - LDG tuner START line (Out6)

![](./assets/ALT_RX_mode_0-2_J4.png)

![](./assets/hl2_options.png)

### Summary
| | Mode 0 | Mode 2 |
|---|---|---|
| RX input | HL2 ANT connector | ALT RX J9 (IO Board) |
| Pure Signal (J10) | Mixed with RX | TX feedback only |
| K2 T/R relay | Normal RX/TX switching | Fixed in RX position |
| Dedicated RX antenna | No | Yes |

### Implementation Notes
- Mode change is detected on every polling cycle (1ms)
- In mode 2, GPIO02_RF3 follows RX/TX state via GPIO13_EXTTR (is_rx) already available in the main loop:
  - RX : RF3 = HIGH → routes J9 ALT RX to HL2
  - TX : RF3 = LOW → routes J10 Pure Signal to HL2
- INTTR (GPIO03) is held HIGH permanently in mode 2 — K2 stays in RX position
- Only `main.c` has been modified for this feature

---

## Credits

- Original IO Board firmware by Jim Ahlstrom N2ADR - https://github.com/jimahlstrom/HL2IOBoard
- Thetis for Hermes Lite 2 by Reid Campbell MI0BOT - https://github.com/mi0bot/OpenHPSDR-Thetis
- LDG AT-1000 Pro II with amp control, BCD band decoder and ALT RX code modifications by Claude Perreault VA2CST - 2026

## License

MIT License - see original project for details
