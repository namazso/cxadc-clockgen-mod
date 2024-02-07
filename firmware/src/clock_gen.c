// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include "dbg.h"
#include "clock_gen.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "libsi5351/si5351.h"

typedef struct
{
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
} pll_setup;

static pll_setup pll_a =
{
	// pll @ 600 MHz integer mode
	.mult = 24,
	.num = 0,
	.denom = 1,
};

static pll_setup pll_b =
{
	// pll @ 859.0908 MHz fractional mode
	.mult = 34,
	.num = 22727,
	.denom = 62500,
};



typedef struct
{
	si5351PLL_t pllSource;
	uint32_t div;
	uint32_t num;
	uint32_t denom;
	si5351RDiv_t r_div;
} multisynth_setup;


#define adc_rate_12mhz 46875
static multisynth_setup setup_12mhz =
{ 
	.pllSource = SI5351_PLL_A,
	.div = 50,
	.num = 0,
	.denom = 1,
	.r_div = SI5351_R_DIV_1,
};
#define adc_rate_12m288hz 48000
static multisynth_setup setup_12m288hz =
{ 
	// 12.288000000 MHz -> PLL B is in fractional mode, and this multisynth also, so high jitter!
	.pllSource = SI5351_PLL_B,
	.div = 69,
	.num = 9349,
	.denom = 10240,
	.r_div = SI5351_R_DIV_1,
};



static multisynth_setup setup_20mhz =
{ 
	.pllSource = SI5351_PLL_A,
	.div = 30,
	.num = 0,
	.denom = 1,
	.r_div = SI5351_R_DIV_1,
};
static multisynth_setup setup_28m6hz =
{ 
	// 28.636360000 MHz -> PLL B is in fractional mode, so high jitter!
	.pllSource = SI5351_PLL_B,
	.div = 30,
	.num = 0,
	.denom = 1,
	.r_div = SI5351_R_DIV_1,
};
static multisynth_setup setup_40mhz =
{ 
	.pllSource = SI5351_PLL_A,
	.div = 15,
	.num = 0,
	.denom = 1,
	.r_div = SI5351_R_DIV_1,
};
static multisynth_setup setup_50mhz =
{ 
	.pllSource = SI5351_PLL_A,
	.div = 12,
	.num = 0,
	.denom = 1,
	.r_div = SI5351_R_DIV_1,
};

static multisynth_setup* setup_cxadc_map[] = 
{
	// NOTE this sequence must match the string descriptors and the sequence of inputs on the switches
	&setup_20mhz,
	&setup_28m6hz,
	&setup_40mhz,
	&setup_50mhz,
};

#define setup_cxadc_map_len (sizeof(setup_cxadc_map) / sizeof(setup_cxadc_map[0]))

static multisynth_setup* ouput0 = NULL;
static multisynth_setup* ouput1 = NULL;
static multisynth_setup* ouput2 = NULL;
static bool init_success;

bool clock_gen_init()
{
	// Initialize I2C port at 100 kHz
	i2c_init(i2c0, 100 * 1000);
	
	// Pins
	const uint sda_pin = 12;
	const uint scl_pin = 13;
	
	// Initialize I2C pins
	gpio_set_function(sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(sda_pin);
	gpio_pull_up(scl_pin);
	
	si5351_init();
	init_success = ( si5351_begin(i2c0) == ERROR_NONE ) ? true : false;
	
	dbg_say("si5351 init ");
	dbg_say(init_success ? "ok" : "failed");
	dbg_say("\n");
	
	ouput0 = NULL;
	ouput1 = NULL;
	ouput2 = NULL;
	return init_success;
}

static bool not_initialized()
{
	if( init_success )
		return false;
	
	dbg_say("si5351 not ready!\n");
	return true;
}

static multisynth_setup* set_multisynth(multisynth_setup* current_config, multisynth_setup* new_config, uint8_t output)
{
	if( (current_config == NULL) \
		|| (current_config->pllSource != new_config->pllSource) \
		|| (current_config->div != new_config->div) \
		|| (current_config->num != new_config->num) \
		|| (current_config->denom != new_config->denom) )
	{
		si5351_setup_multisynth(output, new_config->pllSource, new_config->div, new_config->num, new_config->denom);
	}
	
	if( (current_config == NULL) || (current_config->r_div != new_config->r_div) )
	{
		si5351_setup_rdiv(output, new_config->r_div);
	}
	
	return new_config;
}

void clock_gen_default()
{
	if( not_initialized() ) 
		return;
	
	si5351_setup_pll(SI5351_PLL_A, pll_a.mult, pll_a.num, pll_a.denom);
	si5351_setup_pll(SI5351_PLL_B, pll_b.mult, pll_b.num, pll_b.denom);

	ouput0 = set_multisynth(NULL, &setup_28m6hz, 0);
	ouput1 = set_multisynth(NULL, &setup_28m6hz, 1);
	ouput2 = set_multisynth(NULL, &setup_12m288hz, 2);

	si5351_enable_outputs(true);
}


static const uint32_t adc_rates[] = { adc_rate_12m288hz, adc_rate_12mhz };

const uint32_t* clock_gen_get_adc_sample_rate_options(uint8_t* len)
{
	// https://www.scaler.com/topics/length-of-an-array-in-c/
	*len = sizeof(adc_rates) / sizeof(adc_rates[0]);
	return adc_rates;
}

uint32_t clock_gen_get_adc_sample_rate()
{
	if( not_initialized() ) 
		return 0;
	
	if( ouput2 == &setup_12m288hz )
		return adc_rate_12m288hz;
	if( ouput2 == &setup_12mhz )
		return adc_rate_12mhz;
	
	dbg_say("clock_gen_get_adc_sample_rate unsupported");
	return 0;
}

// https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define xstr(s) str(s)
#define str(s) #s

void clock_gen_set_adc_sample_rate(uint32_t rate_hz)
{
	if( not_initialized() )
		return;
	
	dbg_say("adc=");
	
	if( adc_rate_12m288hz == rate_hz)
	{
		ouput2 = set_multisynth(ouput2, &setup_12m288hz, 2);
		dbg_say(xstr(adc_rate_12m288hz) "\n");
		return;
	}
	
	if( adc_rate_12mhz == rate_hz)
	{
		ouput2 = set_multisynth(ouput2, &setup_12mhz, 2);
		dbg_say(xstr(adc_rate_12mhz) "\n");
		return;
	}
	
	dbg_u32(rate_hz);
	dbg_say("???\n");
}

#undef str
#undef xstr


uint8_t clock_gen_get_cxadc_sample_rate(uint8_t output)
{
	if( not_initialized() ) 
		return 0;
	
	if( output == 0 || output == 1 )
	{
		multisynth_setup* output_settings = (output == 0) ? ouput0 : ouput1;
		
		for( uint8_t i=0; i < setup_cxadc_map_len; ++i)
			if( output_settings == setup_cxadc_map[i] )
				return i;
	}
	
	dbg_say("clock_gen_get_cxadc_sample_rate(");
	dbg_u8(output);
	dbg_say(")???\n");
	return 0;
}

void clock_gen_set_cxadc_sample_rate(uint8_t output, uint8_t frequency_option)
{
	if( not_initialized() ) 
		return;
	
	if( (output == 0 || output == 1) && (frequency_option < setup_cxadc_map_len))
	{
		multisynth_setup* new_settings = setup_cxadc_map[frequency_option];
		
		if( output == 0 )
		{
			ouput0 = set_multisynth(ouput0, new_settings, 0);
		}
		else
		{
			ouput1 = set_multisynth(ouput1, new_settings, 1);
		}
		
		return;
	}
	
	dbg_say("clock_gen_set_cxadc_sample_rate(");
	dbg_u8(output);
	dbg_say(",");
	dbg_u8(frequency_option);
	dbg_say(")???\n");
}

#undef adc_rate_12m288hz
#undef adc_rate_12mhz
