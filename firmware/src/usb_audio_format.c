// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include "usb_audio_format.h"


void usb_audio_pcm24_host_to_usb(uint8_t* buffer, uint32_t data)
{
	// NOTE USB is little endian https://github.com/libopencm3/libopencm3/issues/478
	buffer[0] = data & 0xff; // LSB
	data >>= 8;
	buffer[1] = data & 0xff;
	data >>= 8;
	buffer[2] = data & 0xff; // MSB
}
