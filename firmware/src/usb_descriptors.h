// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ha Thach (tinyusb.org)
// Copyright (c) 2023 Rene Wolf
// Copyright (c) 2024 namazso <admin@namazso.eu>

#ifndef _USB_DESCRIPTORS_H
#define _USB_DESCRIPTORS_H

// Input terminal (line input)
#define USB_DESCRIPTORS_ID_INPUT_PCM1802 0x01
// The switch to select debug output or normal audio output
#define USB_DESCRIPTORS_ID_FEATURE_AUDIO 0x03
// Output terminal (USB)
#define USB_DESCRIPTORS_ID_OUTPUT        0x04
// Clock Source units
#define USB_DESCRIPTORS_ID_CLOCK         0x05

// Fake signal path units
#define USB_DESCRIPTORS_ID_INPUT_40      0x12

#define USB_DESCRIPTORS_ID_OUTPUT_CLK0   0x30
#define USB_DESCRIPTORS_ID_OUTPUT_CLK1   0x31


#define USB_DESCRIPTOR_SERIAL_LEN 16
void usb_descriptor_set_serial(const char* serial);

/* 4.7.2.7 Selector Unit Descriptor */
#define TUD_AUDIO_DESC_SELECTOR_UNIT_4_LEN (7+4)
#define TUD_AUDIO_DESC_SELECTOR_UNIT_4(_bUnitID, _baSourceID1, _baSourceID2, _baSourceID3, _baSourceID4, _bmControls, _iSelector) \
	TUD_AUDIO_DESC_SELECTOR_UNIT_4_LEN, \
	TUSB_DESC_CS_INTERFACE, \
	AUDIO_CS_AC_INTERFACE_SELECTOR_UNIT, \
	_bUnitID, \
	4, \
	_baSourceID1, _baSourceID2, _baSourceID3, _baSourceID4, \
	_bmControls, \
	_iSelector

/* 4.7.2.8 Feature Unit Descriptor */
#define TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN (6+(3+1)*4)
#define TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL(_unitid, _srcid, _ctrlch0master, _ctrlch1, _ctrlch2, _ctrlch3, _stridx) \
	TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN, \
	TUSB_DESC_CS_INTERFACE, \
	AUDIO_CS_AC_INTERFACE_FEATURE_UNIT, \
	_unitid, \
	_srcid, \
	U32_TO_U8S_LE(_ctrlch0master), \
	U32_TO_U8S_LE(_ctrlch1), U32_TO_U8S_LE(_ctrlch2), U32_TO_U8S_LE(_ctrlch3), \
	_stridx


#define TUD_AUDIO_DESC_CS_AC_LEN_TOTAL ( \
	TUD_AUDIO_DESC_CLK_SRC_LEN \
	+ TUD_AUDIO_DESC_INPUT_TERM_LEN \
	+ TUD_AUDIO_DESC_FEATURE_UNIT_THREE_CHANNEL_LEN \
	+ TUD_AUDIO_DESC_OUTPUT_TERM_LEN \
	\
	+ TUD_AUDIO_DESC_INPUT_TERM_LEN \
	)

#define TUD_AUDIO_DESC_TOTAL_LEN ( \
	TUD_AUDIO_DESC_IAD_LEN \
	+ TUD_AUDIO_DESC_STD_AC_LEN \
		+ TUD_AUDIO_DESC_CS_AC_LEN \
			+ TUD_AUDIO_DESC_CS_AC_LEN_TOTAL \
		+ TUD_AUDIO_DESC_STD_AS_INT_LEN \
		+ TUD_AUDIO_DESC_STD_AS_INT_LEN \
	+ TUD_AUDIO_DESC_CS_AS_INT_LEN \
		+ TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN \
		+ TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN \
		+ TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN \
	)

#endif
