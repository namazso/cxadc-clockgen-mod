// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _USB_AUDIO_FORMAT_H
#define _USB_AUDIO_FORMAT_H

#include <stdint.h>
#include <assert.h>

// NOTE this buffer size is slightly larger than 1 ms (46 or 48 samples), but less than 2 ms
//   This MUST be aligned with the isochornous polling rate in the USB desccriptor, which should be set to 1 ms.
//   This will result in polling slightly faster than packets are being produced, and so we will never overflow our buffer.
#define USB_AUDIO_SAMPLES_PER_BUFFER 64
#define USB_AUDIO_BYTES_PER_SAMPLE   3
#define USB_AUDIO_CHANNELS           3
#define USB_AUDIO_PAYLOAD_SIZE       (USB_AUDIO_BYTES_PER_SAMPLE * USB_AUDIO_CHANNELS * USB_AUDIO_SAMPLES_PER_BUFFER)

typedef struct
{
	uint8_t data[USB_AUDIO_PAYLOAD_SIZE];
} usb_audio_buffer;

#define USB_AUDIO_PCM24_MAX  0x007fffff
#define USB_AUDIO_PCM24_MIN  0x00800000
#define USB_AUDIO_PCM24_MASK 0x00ffffff

void usb_audio_pcm24_host_to_usb(uint8_t*buffer, uint32_t data);

#endif

