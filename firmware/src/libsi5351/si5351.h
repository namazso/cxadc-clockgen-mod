// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2014 Adafruit Industries
// Copyright (c) 2023 Rene Wolf

#ifndef _SI5351_H
#define _SI5351_H

#include <stdint.h>
#include "errors.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

typedef enum {
  SI5351_MULTISYNTH_DIV_4 = 4,
  SI5351_MULTISYNTH_DIV_6 = 6,
  SI5351_MULTISYNTH_DIV_8 = 8
} si5351MultisynthDiv_t;

typedef enum {
  SI5351_PLL_A = 0,
  SI5351_PLL_B,
} si5351PLL_t;

typedef enum {
  SI5351_R_DIV_1 = 0,
  SI5351_R_DIV_2 = 1,
  SI5351_R_DIV_4 = 2,
  SI5351_R_DIV_8 = 3,
  SI5351_R_DIV_16 = 4,
  SI5351_R_DIV_32 = 5,
  SI5351_R_DIV_64 = 6,
  SI5351_R_DIV_128 = 7,
} si5351RDiv_t;


// Initializes all values to defaults, the constructor
void  si5351_init();

// Initializes I2C and configures the breakout (call this function before doing anything else)
// i2c - The I2C bus to use.
// returns ERROR_NONE on success
err_t si5351_begin(i2c_inst_t* i2c);                                                                              

// Sets the multiplier for the specified PLL using integer values
// pll  - The PLL to configure
// mult - The PLL integer multiplier of the base 25 MHz frequency.
//        Must be between 15 = 375 Mhz and 90 = 2250 Mhz, but AN619 suggests 600..900MHz as a useable range.
// returns ERROR_NONE on success
err_t si5351_setup_pll_int(si5351PLL_t pll, uint8_t mult);

// Sets the multiplier for the specified PLL using integer values
// pll   - The PLL to configure
// mult  - The PLL integer multiplier of the base 25 MHz frequency.
//         Must be between 15 = 375 Mhz and 90 = 2250 Mhz, but AN619 suggests 600..900MHz as a useable range.
// num   - The 20-bit numerator for fractional output (0..1,048,575). Set this to '0' for integer output.
// denom - The 20-bit denominator for fractional output (1..1,048,575). Set this to '1' or higher to avoid divider by zero errors.
//
//  fVCO = fXTAL * (a+(b/c))
//  
//  fXTAL = the crystal input frequency
//  a     = an integer between 15 and 90
//  b     = the fractional numerator (0..1,048,575)
//  c     = the fractional denominator (1..1,048,575)
//
// returns ERROR_NONE on success
err_t si5351_setup_pll(si5351PLL_t pll, uint8_t mult, uint32_t num, uint32_t denom);

// Configures the Multisynth divider using integer output.
// output    - The output channel to use (0..2)
// pllSource - The PLL input source to use
// div       - The integer divider for the Multisynth output
// returns ERROR_NONE on success
err_t si5351_setup_multisynth_int(uint8_t output, si5351PLL_t pllSource, si5351MultisynthDiv_t div);

// Configures the Multisynth divider, which determines the output clock frequency based on the specified PLL input.
// output    - The output channel to use (0..2)
// pllSource - The PLL input source to use, which must be one of:
// div       - The integer divider for the Multisynth output. This value must be between 8 and 900.
// num       - The 20-bit numerator (0..1,048,575).
// denom     - The 20-bit denominator (1..1,048,575).
//
// The multisynth dividers are applied to the specified PLL output,
// and are used to reduce the PLL output to a valid range (500kHz
// to 160MHz). The relationship can be seen in this formula, where
// fVCO is the PLL output frequency and MSx is the multisynth
// divider:
//
//     fOUT = fVCO / MSx
//
// Valid multisynth dividers are 4, 6, or 8 when using integers,
// or any fractional values between 8 + 1/1,048,575 and 900 + 0/1
//
// The following formula is used for the fractional mode divider:
//
//     a + b / c
//
// a = The integer value, which must be 8..900
// b = The fractional numerator (0..1,048,575)
// c = The fractional denominator (1..1,048,575)
//
// returns ERROR_NONE on success
err_t si5351_setup_multisynth(uint8_t output, si5351PLL_t pllSource, uint32_t div, uint32_t num, uint32_t denom);

// Enables or disables spread spectrum
// enabled - Whether spread spectrum output is enabled
// returns ERROR_NONE on success
err_t si5351_enable_spread_spectrum(bool enabled);

// Enables or disables all clock outputs
// enabled - Whether output is enabled
// returns ERROR_NONE on success
err_t si5351_enable_outputs(bool enabled);

// Configures the additional output devider
// output - Which output to configure (0..2)
// div    - The divider ratio to set
// returns ERROR_NONE on success
err_t si5351_setup_rdiv(uint8_t output, si5351RDiv_t div);

#endif

