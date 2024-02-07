// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include "dbg.h"
#include "pcm1802.h"
#include "pcm1802_fmt00.pio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "usb_audio_format.h"

// see also https://www.pjrc.com/pcm1802-breakout-board-needs-hack/
#define PCM1802_POWER_DOWN_PIN 17


// NOTE GPIOs must be consecutive for the PIO to work and in the order of DATA, BITCLK, LRCLK
#define PCM_PIO_ADC0_DATA   18
#define PCM_PIO_ADC0_BITCLK 19
#define PCM_PIO_ADC0_LRCLK  20
// not connected / outputs debug info from PIO
#define PCM_PIO_ADC0_DEBUG  21

static_assert((PCM_PIO_ADC0_DATA + pcm1802_index_data)   == PCM_PIO_ADC0_DATA,   "ADC0 DATA GPIO not where it should be");
static_assert((PCM_PIO_ADC0_DATA + pcm1802_index_bitclk) == PCM_PIO_ADC0_BITCLK, "ADC0 BITCLK GPIO not where it should be");
static_assert((PCM_PIO_ADC0_DATA + pcm1802_index_lrclk)  == PCM_PIO_ADC0_LRCLK,  "ADC0 LRCLK GPIO not where it should be");
static_assert((PCM_PIO_ADC0_DATA + pcm1802_index_dbg)    == PCM_PIO_ADC0_DEBUG,  "ADC0 DEBUG GPIO not where it should be");




static PIO pio;
static uint32_t pio_program_offset;
static uint32_t pio_sm;
uint32_t pcm1802_out_of_sync_drops;
uint32_t pcm1802_rch_tmo_count;
uint32_t pcm1802_rch_tmo_value;

static uint32_t setup_pio(uint32_t pin)
{
	// https://github.com/raspberrypi/pico-examples/blob/a7ad17156bf60842ee55c8f86cd39e9cd7427c1d/pio/clocked_input/clocked_input.pio#L24
	// https://medium.com/geekculture/raspberry-pico-programming-with-pio-state-machines-e4610e6b0f29
	uint32_t sm = pio_claim_unused_sm(pio, true);

	pio_sm_config cfg = pcm1802_fmt00_program_get_default_config(pio_program_offset);


	// Set and initialize the input pins
	sm_config_set_in_pins(&cfg, pin);
	pio_sm_set_consecutive_pindirs(pio, sm, pin, (pcm1802_index_lrclk-pcm1802_index_data)+1, false);
	sm_config_set_jmp_pin(&cfg, pin + pcm1802_index_data);
	
	// Set and initialize the output pins
	sm_config_set_set_pins(&cfg, pin + pcm1802_index_dbg, 1);
	pio_sm_set_consecutive_pindirs(pio, sm, pin + pcm1802_index_dbg, 1, true);
	
	// we shift LEFT as we have MSB first on PCM interface
	sm_config_set_in_shift(&cfg, false, false, 32);
	
	// Connect these GPIOs to this PIO block
	pio_gpio_init(pio, pin + pcm1802_index_data);
	pio_gpio_init(pio, pin + pcm1802_index_bitclk);
	pio_gpio_init(pio, pin + pcm1802_index_lrclk);
	pio_gpio_init(pio, pin + pcm1802_index_dbg);

	// We only receive, so disable the TX FIFO to make the RX FIFO deeper.
	sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_RX);

	// Load our configuration
	pio_sm_init(pio, sm, pio_program_offset, &cfg);
	
	return sm;
}

void pcm_pio_init()
{
	// https://github.com/raspberrypi/pico-examples/blob/a7ad17156bf60842ee55c8f86cd39e9cd7427c1d/pio/clocked_input/clocked_input.c#L45
	pio = pio0;
	pio_program_offset = pio_add_program(pio, &pcm1802_fmt00_program);
	
	pio_sm = setup_pio(PCM_PIO_ADC0_DATA);
	
	pio_sm_set_enabled(pio, pio_sm, true);
}

void pcm1802_init()
{
	gpio_init(PCM1802_POWER_DOWN_PIN);
	gpio_set_dir(PCM1802_POWER_DOWN_PIN, GPIO_OUT);
	pcm1802_power_down();
	pcm1802_out_of_sync_drops = 0;
	pcm1802_rch_tmo_count = 0;
	pcm1802_rch_tmo_value = 0;
	pcm_pio_init();
}

void pcm1802_power_up()
{
	dbg_say("pcm1802_power_up\n");
	gpio_put(PCM1802_POWER_DOWN_PIN, 1);
}

void pcm1802_power_down()
{
	gpio_put(PCM1802_POWER_DOWN_PIN, 0);
	dbg_say("pcm1802_power_down\n");
}


void pcm1802_rx_24bit_uac_pcm_type1(uint8_t* l_3byte, uint8_t* r_3byte)
{
	while(pcm1802_try_rx_24bit_uac_pcm_type1(l_3byte, r_3byte) == false) { }
}

bool pcm1802_try_rx_24bit_uac_pcm_type1(uint8_t* l_3byte, uint8_t* r_3byte)
{
	if( pio_sm_is_rx_fifo_empty(pio, pio_sm) )
		return false;
	
	uint32_t ch_l = pio_sm_get_blocking(pio, pio_sm);
	if( ch_l & 0x01000000 )
	{
		// we got a sample for the right channel -> out of sync, drop sample wait for next one
		++pcm1802_out_of_sync_drops;
		dbg_say("pcm1802 out of sync, drop!\n");
		return false;
	}

	// while the R sample is being decoded in the PIO, we encode the L sample
	usb_audio_pcm24_host_to_usb(l_3byte, ch_l);

	const uint32_t tmo = 0xffff; // measured actual counter values are around 150 till the next sample comes (at 46kHz)
	uint32_t cnt = 0;
	while( pio_sm_is_rx_fifo_empty(pio, pio_sm) )
	{
		++cnt;
		if( cnt > tmo)
		{
			++pcm1802_rch_tmo_count;
			dbg_say("pcm1802 tmo R!\n");
			return false;
		}
	}
	
	uint32_t ch_r = pio_sm_get_blocking(pio, pio_sm);
	usb_audio_pcm24_host_to_usb(r_3byte, ch_r);

	pcm1802_rch_tmo_value = cnt;
	return true;
}

static bool wait_for_pos_edge_on_pin(uint32_t pin)
{
	// this takes long enough to not timeout on 46kHz which is our slowest clock line (LR clock)
	uint32_t tmo = 0xfff;
	
	while( gpio_get(pin) == true )
	{
		--tmo;
		if( tmo == 0)
			return false;
	}
	
	while( gpio_get(pin) == false )
	{
		--tmo;
		if( tmo == 0)
			return false;
	}
	
	return true;
}

bool pcm1802_activity_on_lrck()
{
	return wait_for_pos_edge_on_pin(PCM_PIO_ADC0_LRCLK);
}

bool pcm1802_activity_on_bck()
{
	return wait_for_pos_edge_on_pin(PCM_PIO_ADC0_BITCLK);
}

bool pcm1802_activity_on_data()
{
	return wait_for_pos_edge_on_pin(PCM_PIO_ADC0_DATA);
}
