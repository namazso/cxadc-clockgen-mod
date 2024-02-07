// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf
// Copyright (c) 2024 namazso <admin@namazso.eu>

#include "clock_gen.h"

#include <hardware/clocks.h>
#include <pico/stdlib.h>

#define adc_rate_40mhz 78125

#define CLOCK_PIN 21

bool clock_gen_init()
{
    // slightly underclock the pico at 120 MHz
    set_sys_clock_khz(120000, true);
    gpio_set_dir(CLOCK_PIN, true);
    // output 120/3 = 40 MHz
    clock_gpio_init_int_frac(CLOCK_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 3, 0);
    return true;
}

void clock_gen_default()
{
        return;
}


static const uint32_t adc_rates[] = { adc_rate_40mhz };

const uint32_t* clock_gen_get_adc_sample_rate_options(uint8_t* len)
{
	// https://www.scaler.com/topics/length-of-an-array-in-c/
	*len = sizeof(adc_rates) / sizeof(adc_rates[0]);
	return adc_rates;
}

uint32_t clock_gen_get_adc_sample_rate()
{
        return adc_rate_40mhz;
}

void clock_gen_set_adc_sample_rate(uint32_t rate_hz)
{
}

#undef adc_rate_40mhz
