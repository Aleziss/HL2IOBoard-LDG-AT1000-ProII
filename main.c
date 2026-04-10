// This is firmware for the Hermes Lite 2 IO board designed by Jim Ahlstrom, N2ADR. It is
//   Copyright (c) 2022-2023 James C. Ahlstrom <jahlstr@gmail.com>.
//   It is licensed under the MIT license. See MIT.txt.
//
// Modified by Claude Perreault, VA2CST - Copyright (c) 2026
//   Changes: Added LDG AT-1000 Pro II antenna tuner support and amplifier control on J6 Out5
//
// This firmware outputs the FT817 band voltage on J4 pin 8, controls the LDG AT-1000 Pro II
// antenna tuner via modified Icom AH-4 protocol, and controls an external amplifier on J6 Out5.

#include "../hl2ioboard.h"
#include "../i2c_registers.h"

// These are the major and minor version numbers for firmware. You must set these.
uint8_t firmware_version_major=1;
uint8_t firmware_version_minor=3;

int main()
{
	static uint8_t current_tx_fcode = 0;
	static bool current_is_rx = true;
	static uint8_t tx_band = 0;
	static uint8_t rx_band = 0;
	static bool tuning_done = false;
	uint8_t band, fcode;
	bool is_rx;
	bool change_band;
	uint8_t i;

	stdio_init_all();
	configure_pins(false, true);
	configure_led_flasher();

	while (1) {	// Wait for something to happen
		sleep_ms(1);	// This sets the polling frequency.
		// Control the LDG AT-1000 Pro II antenna tuner.
		// START line is on J6 pin 6 and the KEY line is on J8 pin 2 with 4.7K pull-up to 3.3V
		IcomAh4(GPIO22_Out6, GPIO18_In2);
		// Poll for a changed Tx band, Rx band and T/R change
		change_band = false;
		is_rx = gpio_get(GPIO13_EXTTR);		// true for receive, false for transmit
		if (current_is_rx != is_rx) {
			current_is_rx = is_rx;
			change_band = true;
		}
		// Amplifier control on J6 Out5 (GPIO10_Out5)
		// Out5 HIGH = amplifier active (TX only, not during tuning)
		// Out5 LOW  = amplifier bypassed (RX, during tuning, or until RX confirmed after tuning)
		if (Registers[REG_ANTENNA_TUNER] != 0) {
			tuning_done = true;			// Mark tuning as active
			gpio_put(GPIO10_Out5, 0);		// Bypass amplifier during tuning
		}
		else if (tuning_done && is_rx) {
			tuning_done = false;			// Reset only when RX confirmed after tuning
			gpio_put(GPIO10_Out5, 0);		// Keep bypassed until RX confirmed
		}
		else if (is_rx || tuning_done) {
			gpio_put(GPIO10_Out5, 0);		// Bypass on RX or post-tuning TX
		}
		else {
			gpio_put(GPIO10_Out5, 1);		// TX normal - activate amplifier
		}
		// Poll for a changed Tx frequency. The new_tx_fcode is set in the I2C handler.
		if (current_tx_fcode != new_tx_fcode) {
			current_tx_fcode = new_tx_fcode;
			change_band = true;
			tx_band = fcode2band(current_tx_fcode);		// Convert the frequency code to a band code.
			ft817_band_volts(tx_band);			// Put the band voltage on J4 pin 8.
		}
		// Poll for a change in one of the twelve Rx frequencies. The rx_freq_changed is set in the I2C handler.
		if (rx_freq_changed) {
			rx_freq_changed = false;
			change_band = true;
			if (rx_freq_high == 0)
				rx_band = tx_band;
			else
				rx_band = fcode2band(rx_freq_high);	// Convert the frequency code to a band code.
		}
		// Band decoder output for optional external filters or amplifier band switching
		// Out1, Out2, Out3 on J4/J6 - not used in this configuration but left for future use
		if (change_band) {
			change_band = false;
			if (tx_band == 0)	// Tx band zero is a reset
				band = 0;
			else if (is_rx)
				band = rx_band;
			else
				band = tx_band;
			switch (band) {		// Set some GPIO pins according to the band
			case BAND_40:
			case BAND_15:
				gpio_put(GPIO16_Out1, 1);
				gpio_put(GPIO19_Out2, 0);
				gpio_put(GPIO20_Out3, 0);
				break;
			case BAND_20:
				gpio_put(GPIO16_Out1, 0);
				gpio_put(GPIO19_Out2, 1);
				gpio_put(GPIO20_Out3, 0);
				break;
			case BAND_10:
				gpio_put(GPIO16_Out1, 0);
				gpio_put(GPIO19_Out2, 0);
				gpio_put(GPIO20_Out3, 1);
				break;
			default:	// This includes band zero (reset)
				gpio_put(GPIO16_Out1, 0);
				gpio_put(GPIO19_Out2, 0);
				gpio_put(GPIO20_Out3, 0);
			}
		}
	}
}