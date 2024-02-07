// SPDX-License-Identifier: MIT
// Copyright (c) 2020 Reinhard Panhuber
// Copyright (c) 2023 Rene Wolf

#include "usb_audio_format.h"
#include "usb_audio.h"
#include "usb_descriptors.h"
#include "tusb.h"
#include "fifo.h"
#include "clock_gen.h"
#include "dbg.h"

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
	(void) rhport;
	(void) pBuff;

	// We do not support any set range requests here, only current value requests
	TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	uint8_t ep = TU_U16_LOW(p_request->wIndex);

	(void) channelNum; (void) ctrlSel; (void) ep;

	dbg_say("set_req_ep_cb\n");
	return false; // Not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
	(void) rhport;
	(void) pBuff;

	// We do not support any set range requests here, only current value requests
	TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	uint8_t itf = TU_U16_LOW(p_request->wIndex);

	(void) channelNum; (void) ctrlSel; (void) itf;

	dbg_say("set_req_itf_cb\n");
	return false; // Not implemented
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request, uint8_t *pBuff)
{
	(void) rhport;

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	uint8_t itf = TU_U16_LOW(p_request->wIndex);
	uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

	dbg_say("set_entity(");
	dbg_u8(channelNum);
	dbg_say(",");
	dbg_u8(ctrlSel);
	dbg_say(",");
	dbg_u8(itf);
	dbg_say(",");
	dbg_u8(entityID);
	dbg_say(")\n");

	if ( entityID == USB_DESCRIPTORS_ID_CLOCK )
	{
		if( ctrlSel == AUDIO_CS_CTRL_SAM_FREQ )
		{
			// Request uses format layout 3
			TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_4_t));

			uint32_t sample_rate = (uint32_t) ((audio_control_cur_4_t*) pBuff)->bCur;
			clock_gen_set_adc_sample_rate(sample_rate);
			return true;
		}
	}

	if ( entityID == USB_DESCRIPTORS_ID_SELECT_CLK0 || entityID == USB_DESCRIPTORS_ID_SELECT_CLK1 )
	{
		if( ctrlSel == AUDIO_SU_CTRL_SELECTOR )
		{
			uint8_t frequency_option = (uint8_t) ((audio_control_cur_1_t*) pBuff)->bCur;
			frequency_option -= 1; // usb selection is 1-based, our selection is 0-based
			clock_gen_set_cxadc_sample_rate((entityID == USB_DESCRIPTORS_ID_SELECT_CLK0) ? 0 : 1, frequency_option);
			return true;
		}
	}
	
	if ( entityID == USB_DESCRIPTORS_ID_FEATURE_AUDIO  )
	{
		if( ctrlSel == AUDIO_FU_CTRL_MUTE )
		{
			uint8_t value = (uint8_t) ((audio_control_cur_1_t*) pBuff)->bCur;
			fifo_set_mode((value == 1) ? fifo_mode_debug : fifo_mode_normal);
			return true;
		}
	}

	// Unknown/Unsupported control
	TU_BREAKPOINT();
	return false; // Not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void) rhport;

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	uint8_t ep = TU_U16_LOW(p_request->wIndex);

	(void) channelNum; (void) ctrlSel; (void) ep;

	dbg_say("get_req_ep_cb\n");
	return false; // Not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void) rhport;

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	uint8_t itf = TU_U16_LOW(p_request->wIndex);

	(void) channelNum; (void) ctrlSel; (void) itf;

	dbg_say("req_itf_cb\n");
	return false; // Nt implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void) rhport;

	// Page 91 in UAC2 specification
	uint8_t channelNum = TU_U16_LOW(p_request->wValue);
	uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
	// uint8_t itf = TU_U16_LOW(p_request->wIndex); 			// Since we have only one audio function implemented, we do not need the itf value
	uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

	dbg_say("get_entity(");
	dbg_u8(channelNum);
	dbg_say(",");
	dbg_u8(ctrlSel);
	dbg_say(",");
	dbg_u8(entityID);
	dbg_say(") ");

	if ( entityID == USB_DESCRIPTORS_ID_CLOCK )
	{
		dbg_say("clock ");
		if( ctrlSel == AUDIO_CS_CTRL_SAM_FREQ )
		{
			if( p_request->bRequest == AUDIO_CS_REQ_CUR )
			{
				dbg_say("freq\n");
				uint32_t sampFreq = clock_gen_get_adc_sample_rate();
				return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));
			}
			if( p_request->bRequest == AUDIO_CS_REQ_RANGE )
			{
				uint8_t options_count;
				const uint32_t* options = clock_gen_get_adc_sample_rate_options(&options_count);
				audio_control_range_4_n_t(options_count) sampleFreqRng; // Sample frequency range state
				sampleFreqRng.wNumSubRanges = options_count;
				for(uint8_t i=0; i<options_count; ++i)
				{
					sampleFreqRng.subrange[i].bMin = options[i];
					sampleFreqRng.subrange[i].bMax = options[i];
					sampleFreqRng.subrange[i].bRes = 0;
				}
				dbg_say("freq range\n");
				return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));
			}
		}
		
		if( ctrlSel == AUDIO_CS_CTRL_CLK_VALID )
		{
			uint8_t clkValid = 1; // clock always valid
			dbg_say("valid\n");
			return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));
		}
		
	}

	if ( entityID == USB_DESCRIPTORS_ID_SELECT_CLK0 || entityID == USB_DESCRIPTORS_ID_SELECT_CLK1 )
	{
		if( ctrlSel == AUDIO_SU_CTRL_SELECTOR )
		{
			uint8_t current = clock_gen_get_cxadc_sample_rate( (entityID == USB_DESCRIPTORS_ID_SELECT_CLK0) ? 0 : 1);
			current+=1; // usb selection is 1-based, our selection is 0-based
			dbg_say("cxadc clk ");
			dbg_say((entityID == USB_DESCRIPTORS_ID_SELECT_CLK0) ? "0\n" : "1\n");
			return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &current, sizeof(current));
		}
	}
	
	if ( entityID == USB_DESCRIPTORS_ID_FEATURE_AUDIO )
	{
		if( ctrlSel == AUDIO_FU_CTRL_MUTE )
		{
			// usb true is 1, false is 0
			uint8_t current = (fifo_get_mode() == fifo_mode_debug) ? 1 : 0;
			dbg_say("fifo mode ");
			dbg_u8(current);
			dbg_say("\n");
			return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &current, sizeof(current));
		}
	}

	dbg_say("???\n");
	TU_BREAKPOINT();
	return false; // Not implemented
}

static uint16_t off = 0;
static usb_audio_buffer* audio_buffer = NULL;

static void next_buffer()
{
	if( audio_buffer != NULL)
	{
		if( off < USB_AUDIO_PAYLOAD_SIZE )
			return;
		
		fifo_put_empty(audio_buffer);
	}
	
	off = 0;
	
	audio_buffer = fifo_try_take_filled();
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t func_id, uint8_t ep_in, uint8_t cur_alt_setting)
{
	next_buffer();
	
	if(audio_buffer == NULL)
	{
		tud_audio_write(NULL, 0);
		return true;
	}
	
	uint16_t remain = USB_AUDIO_PAYLOAD_SIZE - off; 
	tud_audio_write(audio_buffer->data + off, remain);
	return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t func_id, uint8_t ep_in, uint8_t cur_alt_setting)
{
	off += n_bytes_copied;
	next_buffer();
	return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
	(void) rhport;
	(void) p_request;

	dbg_say("close_EP\n");
	return true;
}
