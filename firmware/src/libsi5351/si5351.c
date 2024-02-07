// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2014 Adafruit Industries
// Copyright (c) 2023 Rene Wolf

// @file     Adafruit_SI5351.cpp
// 
// @mainpage Adafruit Si5351 Library
// 
// @author   K. Townsend (Adafruit Industries)
//           Rene Wolf
// 
// @brief    Driver for the SI5351 160MHz Clock Gen.
//           Converted to c from the original c++ version over here
//           https://github.com/adafruit/Adafruit_Si5351_Library/
//           Adapted to work with Pi Pico.
// 
// @section  REFERENCES
// 
// Si5351A/B/C Datasheet:
// http://www.silabs.com/Support%20Documents/TechnicalDocs/Si5351.pdf
// 
// Manually Generating an Si5351 Register Map:
// http://www.silabs.com/Support%20Documents/TechnicalDocs/AN619.pdf
// 
// Clock builder app:
// http://www.adafruit.com/downloads/ClockBuilderDesktopSwInstallSi5351.zip
//
// @section license License
// 
// Software License Agreement (BSD License)
// 
// Copyright (c) 2014, Adafruit Industries
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holders nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>
#include <math.h>
#include "si5351.h"
#include "asserts.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"


#define SI5351_ADDRESS (0x60) // Assumes ADDR pin = low
#define SI5351_READBIT (0x01)

/* See http://www.silabs.com/Support%20Documents/TechnicalDocs/AN619.pdf for
 * registers 26..41 */
enum {
  SI5351_REGISTER_0_DEVICE_STATUS = 0,
  SI5351_REGISTER_1_INTERRUPT_STATUS_STICKY = 1,
  SI5351_REGISTER_2_INTERRUPT_STATUS_MASK = 2,
  SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL = 3,
  SI5351_REGISTER_9_OEB_PIN_ENABLE_CONTROL = 9,
  SI5351_REGISTER_15_PLL_INPUT_SOURCE = 15,
  SI5351_REGISTER_16_CLK0_CONTROL = 16,
  SI5351_REGISTER_17_CLK1_CONTROL = 17,
  SI5351_REGISTER_18_CLK2_CONTROL = 18,
  SI5351_REGISTER_19_CLK3_CONTROL = 19,
  SI5351_REGISTER_20_CLK4_CONTROL = 20,
  SI5351_REGISTER_21_CLK5_CONTROL = 21,
  SI5351_REGISTER_22_CLK6_CONTROL = 22,
  SI5351_REGISTER_23_CLK7_CONTROL = 23,
  SI5351_REGISTER_24_CLK3_0_DISABLE_STATE = 24,
  SI5351_REGISTER_25_CLK7_4_DISABLE_STATE = 25,
  SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1 = 42,
  SI5351_REGISTER_43_MULTISYNTH0_PARAMETERS_2 = 43,
  SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3 = 44,
  SI5351_REGISTER_45_MULTISYNTH0_PARAMETERS_4 = 45,
  SI5351_REGISTER_46_MULTISYNTH0_PARAMETERS_5 = 46,
  SI5351_REGISTER_47_MULTISYNTH0_PARAMETERS_6 = 47,
  SI5351_REGISTER_48_MULTISYNTH0_PARAMETERS_7 = 48,
  SI5351_REGISTER_49_MULTISYNTH0_PARAMETERS_8 = 49,
  SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1 = 50,
  SI5351_REGISTER_51_MULTISYNTH1_PARAMETERS_2 = 51,
  SI5351_REGISTER_52_MULTISYNTH1_PARAMETERS_3 = 52,
  SI5351_REGISTER_53_MULTISYNTH1_PARAMETERS_4 = 53,
  SI5351_REGISTER_54_MULTISYNTH1_PARAMETERS_5 = 54,
  SI5351_REGISTER_55_MULTISYNTH1_PARAMETERS_6 = 55,
  SI5351_REGISTER_56_MULTISYNTH1_PARAMETERS_7 = 56,
  SI5351_REGISTER_57_MULTISYNTH1_PARAMETERS_8 = 57,
  SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1 = 58,
  SI5351_REGISTER_59_MULTISYNTH2_PARAMETERS_2 = 59,
  SI5351_REGISTER_60_MULTISYNTH2_PARAMETERS_3 = 60,
  SI5351_REGISTER_61_MULTISYNTH2_PARAMETERS_4 = 61,
  SI5351_REGISTER_62_MULTISYNTH2_PARAMETERS_5 = 62,
  SI5351_REGISTER_63_MULTISYNTH2_PARAMETERS_6 = 63,
  SI5351_REGISTER_64_MULTISYNTH2_PARAMETERS_7 = 64,
  SI5351_REGISTER_65_MULTISYNTH2_PARAMETERS_8 = 65,
  SI5351_REGISTER_66_MULTISYNTH3_PARAMETERS_1 = 66,
  SI5351_REGISTER_67_MULTISYNTH3_PARAMETERS_2 = 67,
  SI5351_REGISTER_68_MULTISYNTH3_PARAMETERS_3 = 68,
  SI5351_REGISTER_69_MULTISYNTH3_PARAMETERS_4 = 69,
  SI5351_REGISTER_70_MULTISYNTH3_PARAMETERS_5 = 70,
  SI5351_REGISTER_71_MULTISYNTH3_PARAMETERS_6 = 71,
  SI5351_REGISTER_72_MULTISYNTH3_PARAMETERS_7 = 72,
  SI5351_REGISTER_73_MULTISYNTH3_PARAMETERS_8 = 73,
  SI5351_REGISTER_74_MULTISYNTH4_PARAMETERS_1 = 74,
  SI5351_REGISTER_75_MULTISYNTH4_PARAMETERS_2 = 75,
  SI5351_REGISTER_76_MULTISYNTH4_PARAMETERS_3 = 76,
  SI5351_REGISTER_77_MULTISYNTH4_PARAMETERS_4 = 77,
  SI5351_REGISTER_78_MULTISYNTH4_PARAMETERS_5 = 78,
  SI5351_REGISTER_79_MULTISYNTH4_PARAMETERS_6 = 79,
  SI5351_REGISTER_80_MULTISYNTH4_PARAMETERS_7 = 80,
  SI5351_REGISTER_81_MULTISYNTH4_PARAMETERS_8 = 81,
  SI5351_REGISTER_82_MULTISYNTH5_PARAMETERS_1 = 82,
  SI5351_REGISTER_83_MULTISYNTH5_PARAMETERS_2 = 83,
  SI5351_REGISTER_84_MULTISYNTH5_PARAMETERS_3 = 84,
  SI5351_REGISTER_85_MULTISYNTH5_PARAMETERS_4 = 85,
  SI5351_REGISTER_86_MULTISYNTH5_PARAMETERS_5 = 86,
  SI5351_REGISTER_87_MULTISYNTH5_PARAMETERS_6 = 87,
  SI5351_REGISTER_88_MULTISYNTH5_PARAMETERS_7 = 88,
  SI5351_REGISTER_89_MULTISYNTH5_PARAMETERS_8 = 89,
  SI5351_REGISTER_90_MULTISYNTH6_PARAMETERS = 90,
  SI5351_REGISTER_91_MULTISYNTH7_PARAMETERS = 91,
  SI5351_REGISTER_092_CLOCK_6_7_OUTPUT_DIVIDER = 92,
  SI5351_REGISTER_149_SPREAD_SPECTRUM_PARAMETERS = 149,
  SI5351_REGISTER_165_CLK0_INITIAL_PHASE_OFFSET = 165,
  SI5351_REGISTER_166_CLK1_INITIAL_PHASE_OFFSET = 166,
  SI5351_REGISTER_167_CLK2_INITIAL_PHASE_OFFSET = 167,
  SI5351_REGISTER_168_CLK3_INITIAL_PHASE_OFFSET = 168,
  SI5351_REGISTER_169_CLK4_INITIAL_PHASE_OFFSET = 169,
  SI5351_REGISTER_170_CLK5_INITIAL_PHASE_OFFSET = 170,
  SI5351_REGISTER_177_PLL_RESET = 177,
  SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE = 183
};

typedef enum {
  SI5351_CRYSTAL_LOAD_6PF = (1 << 6),
  SI5351_CRYSTAL_LOAD_8PF = (2 << 6),
  SI5351_CRYSTAL_LOAD_10PF = (3 << 6)
} si5351CrystalLoad_t;

typedef enum {
  SI5351_CRYSTAL_FREQ_25MHZ = (25000000),
  SI5351_CRYSTAL_FREQ_27MHZ = (27000000)
} si5351CrystalFreq_t;


/*!
 * @brief SI5351 constructor
 */
typedef struct {
  bool initialised;                //!< Initialization status of SI5351
  si5351CrystalFreq_t crystalFreq; //!< Crystal frequency
  si5351CrystalLoad_t crystalLoad; //!< Crystal load capacitors
  uint32_t crystalPPM;             //!< Frequency synthesis
  bool plla_configured;            //!< Phase-locked loop A configured
  uint32_t plla_freq;              //!< Phase-locked loop A frequency
  bool pllb_configured;            //!< Phase-locked loop B configured
  uint32_t pllb_freq;              //!< Phase-locked loop B frequency
} si5351Config_t;



err_t write_8(uint8_t reg, uint8_t value);
err_t read_8(uint8_t reg, uint8_t *value);
err_t write_n(uint8_t* data, uint8_t n);


static i2c_inst_t* pi_i2c_bus;
static si5351Config_t m_si5351Config;
static uint8_t lastRdivValue[3];
  
void si5351_init()
{
	pi_i2c_bus = NULL;
	
	m_si5351Config.initialised = false;
	m_si5351Config.crystalFreq = SI5351_CRYSTAL_FREQ_25MHZ;
	m_si5351Config.crystalLoad = SI5351_CRYSTAL_LOAD_10PF;
	m_si5351Config.crystalPPM = 30;
	m_si5351Config.plla_configured = false;
	m_si5351Config.plla_freq = 0;
	m_si5351Config.pllb_configured = false;
	m_si5351Config.pllb_freq = 0;
	
	for (uint8_t i = 0; i < 3; i++) 
	{
		lastRdivValue[i] = 0;
	}
}

err_t si5351_begin(i2c_inst_t* i2c)
{
	pi_i2c_bus = i2c;
	
	// Wait for SYS_INIT flag to be clear, indicating that device is ready
	uint8_t regval = 0;
	do
	{
		ASSERT_STATUS(read_8(SI5351_REGISTER_0_DEVICE_STATUS, &regval));
	} 
	while (regval >> 7 == 1);
	
	// Disable all outputs setting CLKx_DIS high
	ASSERT_STATUS(write_8(SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, 0xFF));

	// Power down all output drivers
	ASSERT_STATUS(write_8(SI5351_REGISTER_16_CLK0_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_17_CLK1_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_18_CLK2_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_19_CLK3_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_20_CLK4_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_21_CLK5_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_22_CLK6_CONTROL, 0x80));
	ASSERT_STATUS(write_8(SI5351_REGISTER_23_CLK7_CONTROL, 0x80));

	// Set the load capacitance for the XTAL
	ASSERT_STATUS(write_8(SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE, m_si5351Config.crystalLoad));

	// Disable spread spectrum output.
	si5351_enable_spread_spectrum(false);

	// Set interrupt masks as required (see Register 2 description in AN619).
	// By default, ClockBuilder Desktop sets this register to 0x18.
	// Note that the least significant nibble must remain 0x8, but the most
	// significant nibble may be modified to suit your needs.

	// Reset the PLL config fields just in case we call init again
	m_si5351Config.plla_configured = false;
	m_si5351Config.plla_freq = 0;
	m_si5351Config.pllb_configured = false;
	m_si5351Config.pllb_freq = 0;

	// All done!
	m_si5351Config.initialised = true;

	return ERROR_NONE;
}


err_t si5351_setup_pll_int(si5351PLL_t pll, uint8_t mult)
{
	return si5351_setup_pll(pll, mult, 0, 1);
}

err_t si5351_setup_pll(si5351PLL_t pll, uint8_t mult, uint32_t num, uint32_t denom) 
{
	uint32_t P1; // PLL config register P1
	uint32_t P2; // PLL config register P2
	uint32_t P3; // PLL config register P3

	// Basic validation
	ASSERT(m_si5351Config.initialised, ERROR_DEVICENOTINITIALISED);
	ASSERT((mult > 14) && (mult < 91), ERROR_INVALIDPARAMETER);     // mult = 15..90 */
	ASSERT(denom > 0, ERROR_INVALIDPARAMETER);                      // Avoid divide by zero */
	ASSERT(num <= 0xFFFFF, ERROR_INVALIDPARAMETER);                 // 20-bit limit */
	ASSERT(denom <= 0xFFFFF, ERROR_INVALIDPARAMETER);               // 20-bit limit */

	//  Feedback Multisynth Divider Equation
	// 
	//  where: a = mult, b = num and c = denom
	// 
	// P1 register is an 18-bit value using following formula:
	//   P1[17:0] = 128 * mult + floor(128*(num/denom)) - 512
	// 
	// P2 register is a 20-bit value using the following formula:
	//   P2[19:0] = 128 * num - denom * floor(128*(num/denom))
	// 
	// P3 register is a 20-bit value using the following formula:
	//   P3[19:0] = denom
	// 

	// Set the main PLL config registers 
	if (num == 0) 
	{
		// Integer mode
		P1 = 128 * mult - 512;
		P2 = num;
		P3 = denom;
	} 
	else 
	{
		// Fractional mode 
		P1 = (uint32_t)(128 * mult + floor(128 * ((float)num / (float)denom)) - 512);
		P2 = (uint32_t)(128 * num - denom * floor(128 * ((float)num / (float)denom)));
		P3 = denom;
	}

	// Get the appropriate starting point for the PLL registers 
	uint8_t baseaddr = (pll == SI5351_PLL_A ? 26 : 34);

	// The datasheet is a nightmare of typos and inconsistencies here! 
	ASSERT_STATUS(write_8(baseaddr, (P3 & 0x0000FF00) >> 8));
	ASSERT_STATUS(write_8(baseaddr + 1, (P3 & 0x000000FF)));
	ASSERT_STATUS(write_8(baseaddr + 2, (P1 & 0x00030000) >> 16));
	ASSERT_STATUS(write_8(baseaddr + 3, (P1 & 0x0000FF00) >> 8));
	ASSERT_STATUS(write_8(baseaddr + 4, (P1 & 0x000000FF)));
	ASSERT_STATUS(write_8(baseaddr + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16)));
	ASSERT_STATUS(write_8(baseaddr + 6, (P2 & 0x0000FF00) >> 8));
	ASSERT_STATUS(write_8(baseaddr + 7, (P2 & 0x000000FF)));

	// Reset both PLLs 
	ASSERT_STATUS(write_8(SI5351_REGISTER_177_PLL_RESET, (1 << 7) | (1 << 5)));

	// Store the frequency settings for use with the Multisynth helper 
	if (pll == SI5351_PLL_A) 
	{
		float fvco = m_si5351Config.crystalFreq * (mult + ((float)num / (float)denom));
		m_si5351Config.plla_configured = true;
		m_si5351Config.plla_freq = (uint32_t)floor(fvco);
	} 
	else 
	{
		float fvco = m_si5351Config.crystalFreq * (mult + ((float)num / (float)denom));
		m_si5351Config.pllb_configured = true;
		m_si5351Config.pllb_freq = (uint32_t)floor(fvco);
	}

	return ERROR_NONE;
}

err_t si5351_setup_multisynth_int(uint8_t output, si5351PLL_t pllSource, si5351MultisynthDiv_t div)
{
	return si5351_setup_multisynth(output, pllSource, div, 0, 1);
}

err_t si5351_setup_multisynth(uint8_t output, si5351PLL_t pllSource, uint32_t div, uint32_t num, uint32_t denom)
{
	uint32_t P1; // Multisynth config register P1
	uint32_t P2; // Multisynth config register P2
	uint32_t P3; // Multisynth config register P3

	// Basic validation
	ASSERT(m_si5351Config.initialised, ERROR_DEVICENOTINITIALISED);
	ASSERT(output < 3, ERROR_INVALIDPARAMETER);       // Channel range
	ASSERT(div > 3, ERROR_INVALIDPARAMETER);          // Divider integer value
	ASSERT(div < 2049, ERROR_INVALIDPARAMETER);       // Divider integer value
	ASSERT(denom > 0, ERROR_INVALIDPARAMETER);        // Avoid divide by zero
	ASSERT(num <= 0xFFFFF, ERROR_INVALIDPARAMETER);   // 20-bit limit
	ASSERT(denom <= 0xFFFFF, ERROR_INVALIDPARAMETER); // 20-bit limit

	// Make sure the requested PLL has been initialised
	if (pllSource == SI5351_PLL_A)
	{
		ASSERT(m_si5351Config.plla_configured, ERROR_INVALIDPARAMETER);
	} 
	else 
	{
		ASSERT(m_si5351Config.pllb_configured, ERROR_INVALIDPARAMETER);
	}

	//  Output Multisynth Divider Equations
	//
	//  where: a = div, b = num and c = denom
	//
	// P1 register is an 18-bit value using following formula:
	//   P1[17:0] = 128 * a + floor(128*(b/c)) - 512
	//
	// P2 register is a 20-bit value using the following formula:
	//   P2[19:0] = 128 * b - c * floor(128*(b/c))
	//
	// P3 register is a 20-bit value using the following formula:
	//   P3[19:0] = c
	//

	// Set the main PLL config registers
	if (num == 0) 
	{
		// Integer mode
		P1 = 128 * div - 512;
		P2 = 0;
		P3 = denom;
	} 
	else if (denom == 1) 
	{
		// Fractional mode, simplified calculations
		P1 = 128 * div + 128 * num - 512;
		P2 = 128 * num - 128;
		P3 = 1;
	}
	else
	{
		// Fractional mode
		P1 = (uint32_t)(128 * div + floor(128 * ((float)num / (float)denom)) - 512);
		P2 = (uint32_t)(128 * num - denom * floor(128 * ((float)num / (float)denom)));
		P3 = denom;
	}

	// Get the appropriate starting point for the PLL registers
	uint8_t baseaddr = 0;
	switch (output)
	{
		case 0:
			baseaddr = SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1;
			break;
		case 1:
			baseaddr = SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1;
			break;
		case 2:
			baseaddr = SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1;
			break;
	}

	// Set the MSx config registers
	// Burst mode: register address auto-increases
	uint8_t sendBuffer[9];
	sendBuffer[0] = baseaddr;
	sendBuffer[1] = (P3 & 0xFF00) >> 8;
	sendBuffer[2] = P3 & 0xFF;
	sendBuffer[3] = ((P1 & 0x30000) >> 16) | lastRdivValue[output];
	sendBuffer[4] = (P1 & 0xFF00) >> 8;
	sendBuffer[5] = P1 & 0xFF;
	sendBuffer[6] = ((P3 & 0xF0000) >> 12) | ((P2 & 0xF0000) >> 16);
	sendBuffer[7] = (P2 & 0xFF00) >> 8;
	sendBuffer[8] = P2 & 0xFF;
	ASSERT_STATUS(write_n(sendBuffer, 9));

	// Configure the clk control and enable the output
	// TODO: Check if the clk control byte needs to be updated.
	uint8_t clkControlReg = 0x0F; // 8mA drive strength, MS0 as CLK0 source, Clock not inverted, powered up
	if (pllSource == SI5351_PLL_B)
	{
		clkControlReg |= (1 << 5); // Uses PLLB
	}
		
	if (num == 0)
	{
		clkControlReg |= (1 << 6); // Integer mode
	}
	
	switch (output) 
	{
		case 0:
			ASSERT_STATUS(write_8(SI5351_REGISTER_16_CLK0_CONTROL, clkControlReg));
			break;
		case 1:
			ASSERT_STATUS(write_8(SI5351_REGISTER_17_CLK1_CONTROL, clkControlReg));
			break;
		case 2:
			ASSERT_STATUS(write_8(SI5351_REGISTER_18_CLK2_CONTROL, clkControlReg));
			break;
	}

	return ERROR_NONE;
}

err_t si5351_setup_rdiv(uint8_t output, si5351RDiv_t div)
{
	ASSERT(output < 3, ERROR_INVALIDPARAMETER); // Channel range

	uint8_t Rreg, regval;

	if (output == 0)
	{
		Rreg = SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3;
	}
	
	if (output == 1)
	{
		Rreg = SI5351_REGISTER_52_MULTISYNTH1_PARAMETERS_3;
	}
	
	if (output == 2)
	{
		Rreg = SI5351_REGISTER_60_MULTISYNTH2_PARAMETERS_3;
	}

	read_8(Rreg, &regval);

	regval &= 0x0F;
	uint8_t divider = div;
	divider &= 0x07;
	divider <<= 4;
	regval |= divider;
	lastRdivValue[output] = divider;
	return write_8(Rreg, regval);
}

err_t si5351_enable_outputs(bool enabled)
{
	// Make sure we've called init first
	ASSERT(m_si5351Config.initialised, ERROR_DEVICENOTINITIALISED);

	// Enabled desired outputs (see Register 3)
	ASSERT_STATUS(write_8(SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, enabled ? 0x00 : 0xFF));

	return ERROR_NONE;
}

err_t si5351_enable_spread_spectrum(bool enabled)
{
	uint8_t regval;
	
	ASSERT_STATUS(read_8(SI5351_REGISTER_149_SPREAD_SPECTRUM_PARAMETERS, &regval));
	
	if (enabled) 
	{
		regval |= 0x80;
	} 
	else 
	{
		regval &= ~0x80;
	}
	
	ASSERT_STATUS(write_8(SI5351_REGISTER_149_SPREAD_SPECTRUM_PARAMETERS, regval));

	return ERROR_NONE;
}

// I2C stuff

err_t write_8(uint8_t reg, uint8_t value)
{
	// Registers always have a 8-bit address and 8-bit value
	// Si5351 data sheet Figure 10
	
	uint8_t buffer[2];
	buffer[0] = reg;
	buffer[1] = value;
	
	int written = i2c_write_blocking(pi_i2c_bus, SI5351_ADDRESS, buffer, sizeof(buffer), false);
	
	return written == sizeof(buffer) ? ERROR_NONE : ERROR_I2C_TRANSACTION;
}

err_t read_8(uint8_t reg, uint8_t *value)
{
	// Registers always have a 8-bit address and 8-bit value
	// Si5351 data sheet Figure 10
	uint8_t written = i2c_write_blocking(pi_i2c_bus, SI5351_ADDRESS, &reg, 1, false);
	if( written != 1)
		return 0;
	
	int read = i2c_read_blocking(pi_i2c_bus, SI5351_ADDRESS, value, 1, false);
	
	return read == 1 ? ERROR_NONE : ERROR_I2C_TRANSACTION;
}

err_t write_n(uint8_t* data, uint8_t n)
{
	int written = i2c_write_blocking(pi_i2c_bus, SI5351_ADDRESS, data, n, false);
	return written == n ? ERROR_NONE : ERROR_I2C_TRANSACTION;
}
