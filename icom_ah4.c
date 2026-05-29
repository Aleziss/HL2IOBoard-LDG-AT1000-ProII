// This is firmware for the Hermes Lite 2 IO board designed by Jim Ahlstrom, N2ADR. It is
//   Copyright (c) 2022-2025 James C. Ahlstrom <jahlstr@gmail.com>.
//   It is licensed under the MIT license. See MIT.txt.
//
// Modified by Claude Perreault, VA2CST - Copyright (c) 2026
//   Changes: Rewrote IcomAh4() to support LDG AT-1000 Pro II timing behavior
//            which differs from standard Icom AH-4 protocol
//
// This implements control for the LDG AT-1000 Pro II antenna tuner (firmware v1.7) using Icom AH-4 compatible protocol.
// The LDG tuner behavior differs from the standard AH-4 protocol:
//   - START must be held high >500ms and <2.5s to trigger a tuning sequence (not just a pulse)
//   - KEY goes low AFTER START is released (not while START is high as in standard AH-4)
//   - KEY remains low while the tuner is waiting for RF
//   - KEY goes high when tuning is complete
//
// Wiring:
//   DO NOT POWER THE LDG TUNER WITH THE IO BOARD 12V SWITCHED LINE, USE EXTERNAL PSU
//   START line (ring) to J6 pin 6 (GPIO22_Out6 low-side switch)
//   KEY line (tip) to J8 pin 2 (GPIO18_In2) with 4.7Kohm 5V pull-up resistor (P3 on IO Board)
//   GND line (sleeve) to GND (G1 or G2 on IO Board)
//
// Tuning sequence:
//   1. Thetis sends tune request (writes 1 to REG_ANTENNA_TUNER via CTRL+TUN)
//   2. Pico verifies KEY is high before starting - returns 0xFA if not
//   3. Pico asserts START high for 600ms then releases
//   4. LDG pulls KEY low to indicate ready for RF
//   5. Pico sets REG_ANTENNA_TUNER to 0xEE to request RF from Thetis (10-25 watts required for LDG AT-1000 Pro II)
//   6. Thetis transmits CW tune signal
//   7. LDG tunes the antenna and pulls KEY high when complete
//   8. Pico sets REG_ANTENNA_TUNER to 0 to stop RF
//   9. Safety timeout of 20 seconds prevents indefinite transmission
//
// Error codes:
//   0x00 - Success
//   0xFA - KEY not high at start - check wiring and pull-up resistor
//   0xFB - Timeout - KEY never went low after START released
//   0xFD - Safety timeout - tuning exceeded 20 seconds

#include "../hl2ioboard.h"
#include "../i2c_registers.h"

void IcomAh4(uint8_t AH4_START, uint8_t AH4_KEY)
{
    static uint8_t ldg_state = 0;     // Current state of the tuning state machine
    static absolute_time_t ldg_timer; // Timer for state transitions

    switch (ldg_state)
    {
    case 0: // Idle - wait for tune command from Thetis (REG_ANTENNA_TUNER == 1)
        if (Registers[REG_ANTENNA_TUNER] == 1)
        {
            if (gpio_get(AH4_KEY) == 0)
            {
                Registers[REG_ANTENNA_TUNER] = 0xFA; // Error - KEY not high, check wiring
                ldg_state = 0;                       // Stay idle
            }
            else
            {
                gpio_put(AH4_START, 1);          // Assert START high to begin tuning sequence
                ldg_state = 1;                   // Go to START timer state
                ldg_timer = get_absolute_time(); // Start 600ms timer
            }
        }
        break;
    case 1: // Hold START high for 600ms then release to trigger LDG tuning sequence
        if (absolute_time_diff_us(ldg_timer, get_absolute_time()) / 1000 >= 600)
        {
            gpio_put(AH4_START, 0);          // Release START - LDG will pull KEY low in response
            ldg_state = 2;                   // Go to KEY-low waiting state
            ldg_timer = get_absolute_time(); // Start 2000ms timeout timer
        }
        break;
    case 2: // Wait for KEY to go low - LDG signals ready for RF after START is released
        if (gpio_get(AH4_KEY) == 0)
        {
            Registers[REG_ANTENNA_TUNER] = 0xEE; // Instruct Thetis to begin CW Tx (10-25 watts required for LDG AT-1000 Pro II)
            ldg_state = 3;                       // Go to RF active state
            ldg_timer = get_absolute_time();     // Start 20 second safety timer
        }
        else if (absolute_time_diff_us(ldg_timer, get_absolute_time()) / 1000 >= 2000)
        {
            ldg_state = 0;                       // Return to idle
            Registers[REG_ANTENNA_TUNER] = 0xFB; // Report timeout - KEY never went low
        }
        break;
    case 3: // RF active - wait for KEY to go high indicating tuning complete
            // Safety timeout of 20 seconds prevents indefinite transmission
        if (gpio_get(AH4_KEY))
        {
            Registers[REG_ANTENNA_TUNER] = 0; // Tuning complete - instruct Thetis to stop RF
            ldg_state = 0;                    // Return to idle
        }
        else if (absolute_time_diff_us(ldg_timer, get_absolute_time()) / 1000 >= 20000)
        {
            Registers[REG_ANTENNA_TUNER] = 0xFD; // Safety timeout - force Thetis to stop RF
            ldg_state = 0;                       // Return to idle
        }
        break;
    }
}