// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include <string.h>
#include "dbg.h"
#include "pico/stdlib.h"
#include "main1.h"
#include "fifo.h"
#include "usb_audio_format.h"
#include "pcm1802.h"
#include "head_switch.h"
#include "global_status.h"

// The exact value does not matter, it just has to be large enough to not run out
// between two regular sample values. A value of 0xffff will timout about 100 times per second
#define TIMEOUT_COUNT_DOWN 0xffff

static bool fill_buffer_normal(usb_audio_buffer* buffer)
{
	for(int i=0; i<USB_AUDIO_SAMPLES_PER_BUFFER; ++i)
	{
		uint8_t* current_frame = buffer->data + ( i * USB_AUDIO_CHANNELS * USB_AUDIO_BYTES_PER_SAMPLE );
		uint32_t tmo = 0; 

		// left goes into ch0, right into ch1
		while( pcm1802_try_rx_24bit_uac_pcm_type1(current_frame, current_frame + USB_AUDIO_BYTES_PER_SAMPLE ) == false)
		{
			++tmo; // no new data in buffer, increment our timeout countdown
			if( tmo > TIMEOUT_COUNT_DOWN )
			{
				global_status_access( global_status.main1_rxsample_tmo += 1 );
				return false; // reached the timeout something is really wrong, we quit ...
			}
		}

		// head switch / sync pin goes into ch2
		uint32_t pin_pcm_value = head_switch_sample_pin() ? USB_AUDIO_PCM24_MAX : USB_AUDIO_PCM24_MIN;
		usb_audio_pcm24_host_to_usb(current_frame + (2*USB_AUDIO_BYTES_PER_SAMPLE), pin_pcm_value);
	}
	
	global_status_access(
	{
		// as we just got an entire frame from the ADC we can safely assume we got activity on all pins
		global_status.pcm1802_activity_lrck = true_u8;
		global_status.pcm1802_activity_bck = true_u8;
		global_status.pcm1802_activity_data = true_u8;
		// after actually getting some samples we update the pcm counters
		global_status.pcm1802_out_of_sync_drops = pcm1802_out_of_sync_drops;
		global_status.pcm1802_rch_tmo_count = pcm1802_rch_tmo_count;
		global_status.pcm1802_rch_tmo_value = pcm1802_rch_tmo_value;
	});
	
	return true;
}

static bool fill_buffer_debug(usb_audio_buffer* buffer)
{
	memset(buffer->data, 0, USB_AUDIO_PAYLOAD_SIZE);
	
	int off = 0;
	
	uint32_t header = GLOBAL_STATUS_MAGIC_NUMBER;
	memcpy( (buffer->data) + off, &header, sizeof(uint32_t) );
	off += sizeof(uint32_t);
	
	// Make sure our status structure actually fits in one frame (it really should but just to be safe)
	int size = sizeof(global_status_fields);
	int left = USB_AUDIO_PAYLOAD_SIZE - off;
	if( size > left)
		size = left;

	// NOTE as these activity checks may take a while to perfrom, we do them OUTSIDE of the access lock
	bool act_bck = pcm1802_activity_on_bck();
	bool act_lrck = pcm1802_activity_on_lrck();
	bool act_data = pcm1802_activity_on_data();
	
	global_status_access(
	{
		global_status.pcm1802_activity_bck  = global_status_to_boolu8(act_bck);
		global_status.pcm1802_activity_lrck = global_status_to_boolu8(act_lrck);
		global_status.pcm1802_activity_data = global_status_to_boolu8(act_data);
		memcpy( (buffer->data) + off, &global_status, size );
	});
	
	return true;
}

static void fill_buffer(usb_audio_buffer* buffer)
{
	while(true)
	{
		fifo_mode mode = fifo_get_mode();
		bool success = false;
		
		if( mode == fifo_mode_normal )
			success = fill_buffer_normal( buffer );
		
		if( mode == fifo_mode_debug )
			success = fill_buffer_debug( buffer );
		
		if( success )
			return;
	}
}

void main1()
{
	dbg_say("main1()\n");
	
	head_switch_init();
	pcm1802_init();
	pcm1802_power_up();
	
	while(1)
	{
		usb_audio_buffer* buffer = fifo_take_empty();
		fill_buffer(buffer);
		fifo_put_filled(buffer);
	}
}
