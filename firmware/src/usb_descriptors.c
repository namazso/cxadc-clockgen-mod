// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ha Thach (tinyusb.org)
// Copyright (c) 2023 Rene Wolf

#include "tusb.h"
#include "tusb_config.h"
#include "usb_descriptors.h"
#include "dbg.h"
#include "clock_gen.h"
#include "build_info.h"

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static char usb_serial_number[USB_DESCRIPTOR_SERIAL_LEN + 1];

void usb_descriptor_set_serial(const char* serial)
{
	int count = strlen(serial);
	if( count > USB_DESCRIPTOR_SERIAL_LEN )
		count = USB_DESCRIPTOR_SERIAL_LEN;
	
	memcpy(usb_serial_number, serial, count);
	usb_serial_number[count]=0;
}

// array of pointer to string descriptors
char const* string_desc_arr [] =
{
	#define STRD_IDX_NONE           0
	#define STRD_IDX_LANG           0
	(const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
	
	#define STRD_IDX_MANUFACTURER   1
	"Rene Wolf",
	
	#define STRD_IDX_PRODUCT        2
	"CXADC+ADC-ClockGen",
	
	#define STRD_IDX_SERIAL         3
	usb_serial_number,
	
	#define STRD_IDX_VERSION        4
	"v" NFO_SEMVER_STR,

	#define STRD_IDX_INPUT_PCM1802  5
	"ADC + Head switch",
	#define STRD_IDX_FEATURE_ADUIO  6
	"Audio Control",
	
	#define STRD_IDX_INPUT_20       7
	"CXADC-"CLOCK_GEN_CXADC_CLOCK_F0_STR,
	#define STRD_IDX_INPUT_28       8
	"CXADC-"CLOCK_GEN_CXADC_CLOCK_F1_STR,
	#define STRD_IDX_INPUT_40       9
	"CXADC-"CLOCK_GEN_CXADC_CLOCK_F2_STR,
	#define STRD_IDX_INPUT_50       10
	"CXADC-"CLOCK_GEN_CXADC_CLOCK_F3_STR,
	
	#define STRD_IDX_SELECT_0       11
	"CXADC-Clock 0 Select",
	#define STRD_IDX_SELECT_1       12
	"CXADC-Clock 1 Select",
	
	#define STRD_IDX_OUT_0          13
	"CXADC-Clock 0 Out",
	#define STRD_IDX_OUT_1          14
	"CXADC-Clock 1 Out",
};

#define STRING_DESCRIPTOR_BUFFER 32
static uint16_t _desc_str[STRING_DESCRIPTOR_BUFFER];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
	(void) langid;

	uint8_t chr_count;

	if ( index == 0)
	{
		// Index 0 is special, no UTF-16 conversion needed
		memcpy(&_desc_str[1], string_desc_arr[0], 2);
		chr_count = 1;
	}
	else
	{
		// Convert ASCII string into UTF-16

		if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) 
			return NULL;

		const char* str = string_desc_arr[index];

		// Cap at max char
		chr_count = strlen(str);
		if ( chr_count > (STRING_DESCRIPTOR_BUFFER - 1) ) chr_count = (STRING_DESCRIPTOR_BUFFER - 1);

		for(uint8_t i=0; i<chr_count; i++)
		{
			_desc_str[1+i] = str[i];
		}
	}

	// first byte is length (including header), second byte is string type
	_desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

	dbg_say("str_d ");
	dbg_u8(index);
	dbg_say("\n");
	return _desc_str;
}

#undef STRING_DESCRIPTOR_BUFFER

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
	.bLength            = sizeof(tusb_desc_device_t),
	.bDescriptorType    = TUSB_DESC_DEVICE,
	.bcdUSB             = 0x0200,

	// Use Interface Association Descriptor (IAD) for CDC
	// As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
	.bDeviceClass       = TUSB_CLASS_MISC,
	.bDeviceSubClass    = MISC_SUBCLASS_COMMON,
	.bDeviceProtocol    = MISC_PROTOCOL_IAD,
	.bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,


#warning THIS CODE IS USING A TEST PID, DO NOT REDISTRIBUTE!!! see here https://pid.codes/1209/0001/
	// TODO request one here if this thing ever works https://pid.codes/howto/
	.idVendor           = 0x1209,
	.idProduct          = 0x0001,
	.bcdDevice          = NFO_SEMVER_USB_DEV_BCD,

	.iManufacturer      = STRD_IDX_MANUFACTURER,
	.iProduct           = STRD_IDX_PRODUCT,
	.iSerialNumber      = STRD_IDX_SERIAL,

	.bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
	return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#if TU_CHECK_MCU(LPC175X_6X) || TU_CHECK_MCU(LPC177X_8X) || TU_CHECK_MCU(LPC40XX)
	// LPC 17xx and 40xx endpoint type (bulk/interrupt/iso) are fixed by its number
	// 0 control, 1 In, 2 Bulk, 3 Iso, 4 In etc ...
	#define EPNUM_AUDIO   0x03

#elif TU_CHECK_MCU(NRF5X)
	// nRF5x ISO can only be endpoint 8
	#define EPNUM_AUDIO   0x08

#else
	#define EPNUM_AUDIO   0x01
#endif

enum
{
	ITF_NUM_AUDIO_CONTROL = 0,
	ITF_NUM_AUDIO_STREAMING,
	ITF_NUM_TOTAL
};

#define AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED  0x0700
#define AUDIO_TERM_TYPE_IN_EXTERNAL_LINE       0x0603

// Adapted from TUD_AUDIO_MIC_FOUR_CH_DESCRIPTOR
uint8_t const desc_configuration[] =
{
	// Interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, (TUD_CONFIG_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_DESC_TOTAL_LEN), 0x00, 100),
	
	/* Standard Interface Association Descriptor (IAD) */\
	TUD_AUDIO_DESC_IAD(/*_firstitfs*/ ITF_NUM_AUDIO_CONTROL, /*_nitfs*/ 0x02, /*_stridx*/ STRD_IDX_VERSION),\
	
		/* Standard AC Interface Descriptor(4.7.1) */\
		TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ ITF_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x00, /*_stridx*/ STRD_IDX_PRODUCT /* NOTE windows reports this as the device name, linux / ALSA uses the product everywhere */),\
			/* Class-Specific AC Interface Header Descriptor(4.7.2) */\
			TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_IO_BOX, /*_totallen*/ TUD_AUDIO_DESC_CS_AC_LEN_TOTAL, /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
			/* Clock Source Descriptor(4.7.2.1) */\
			TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_attr*/ AUDIO_CLOCK_SOURCE_ATT_INT_PRO_CLK, /*_ctrl*/ (AUDIO_CTRL_RW << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS | AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_VAL_POS), /*_assocTerm*/ 0,  /*_stridx*/ 0x00),\
			/* Input Terminal Descriptor(4.7.2.4) */\
			TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_INPUT_PCM1802, /*_termtype*/ AUDIO_TERM_TYPE_IN_EXTERNAL_LINE, /*_assocTerm*/ 0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_nchannelslogical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_INPUT_PCM1802),\
			/* Feature Unit Descriptor (4.7.2.8)*/ \
			TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL(/*_unitid*/ USB_DESCRIPTORS_ID_FEATURE_AUDIO, /*_srcid*/ USB_DESCRIPTORS_ID_INPUT_PCM1802, /*_ctrlch0master*/ (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS), /*_ctrlch1*/ 0, /*_ctrlch2*/ 0, /*_ctrlch3*/ 0, /*_stridx*/ STRD_IDX_FEATURE_ADUIO), \
			/* Output Terminal Descriptor(4.7.2.5) */\
			TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_OUTPUT, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0, /*_srcid*/ USB_DESCRIPTORS_ID_FEATURE_AUDIO, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
			\
			TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_INPUT_20, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_nchannelslogical*/ 1, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_INPUT_20),\
			TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_INPUT_28, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_nchannelslogical*/ 1, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_INPUT_28),\
			TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_INPUT_40, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_nchannelslogical*/ 1, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_INPUT_40),\
			TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_INPUT_50, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_nchannelslogical*/ 1, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_INPUT_50),\
			TUD_AUDIO_DESC_SELECTOR_UNIT_4(/*_bUnitID*/ USB_DESCRIPTORS_ID_SELECT_CLK0, /*_baSourceID1*/ USB_DESCRIPTORS_ID_INPUT_20, /*_baSourceID2*/ USB_DESCRIPTORS_ID_INPUT_28, /*_baSourceID3*/ USB_DESCRIPTORS_ID_INPUT_40, /*_baSourceID4*/ USB_DESCRIPTORS_ID_INPUT_50, /*_bmControls*/ (AUDIO_CTRL_RW << 0), /*_iSelector*/ STRD_IDX_SELECT_0), \
			TUD_AUDIO_DESC_SELECTOR_UNIT_4(/*_bUnitID*/ USB_DESCRIPTORS_ID_SELECT_CLK1, /*_baSourceID1*/ USB_DESCRIPTORS_ID_INPUT_20, /*_baSourceID2*/ USB_DESCRIPTORS_ID_INPUT_28, /*_baSourceID3*/ USB_DESCRIPTORS_ID_INPUT_40, /*_baSourceID4*/ USB_DESCRIPTORS_ID_INPUT_50, /*_bmControls*/ (AUDIO_CTRL_RW << 0), /*_iSelector*/ STRD_IDX_SELECT_1), \
			TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_OUTPUT_CLK0, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_srcid*/ USB_DESCRIPTORS_ID_SELECT_CLK0, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_OUT_0),\
			TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ USB_DESCRIPTORS_ID_OUTPUT_CLK1, /*_termtype*/ AUDIO_TERM_TYPE_IO_EMBEDDED_UNDEFINED, /*_assocTerm*/ 0, /*_srcid*/ USB_DESCRIPTORS_ID_SELECT_CLK1, /*_clkid*/ USB_DESCRIPTORS_ID_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ STRD_IDX_OUT_1),\
			
	
		/* Standard AS Interface Descriptor(4.9.1) */\
		/* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
		TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING, /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),\
		/* Standard AS Interface Descriptor(4.9.1) */\
		/* Interface 1, Alternate 1 - alternate interface for data streaming */\
		TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ ITF_NUM_AUDIO_STREAMING, /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),\
	
			/* Class-Specific AS Interface Descriptor(4.9.2) */\
			TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ USB_DESCRIPTORS_ID_OUTPUT, /*_ctrl*/ AUDIO_CTRL_NONE, /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
			/* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
			TUD_AUDIO_DESC_TYPE_I_FORMAT(USB_AUDIO_BYTES_PER_SAMPLE, (USB_AUDIO_BYTES_PER_SAMPLE*8)),\

			/* "bInterval is used to specify the polling interval [...] expressed in frames, thus this equates to either 1ms for low/full speed devices and 125us for high speed devices." src: https://www.beyondlogic.org/usbnutshell/usb5.shtml */ \
			/* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
			TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ (0x80 | EPNUM_AUDIO), /*_attr*/ (TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ CFG_TUD_AUDIO_EP_SZ_IN, /*_interval*/ (CFG_TUSB_RHPORT0_MODE & OPT_MODE_HIGH_SPEED) ? 0x08 : 0x01),\
				/* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
				TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
	(void) index; // for multiple configurations
	return desc_configuration;
}
