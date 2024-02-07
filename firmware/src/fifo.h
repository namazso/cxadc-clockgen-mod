// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _FIFO_H
#define _FIFO_H

#include <stdint.h>
#include "usb_audio_format.h"

// how much entries of space the fifo should have
#define FIFO_SPACE 8

// initializes the fifo and its buffers. all buffers will be cleared and put into the empty queue
void fifo_init();

usb_audio_buffer* fifo_take_empty();
usb_audio_buffer* fifo_try_take_empty();
void              fifo_put_empty(usb_audio_buffer* buffer);

usb_audio_buffer* fifo_take_filled();
usb_audio_buffer* fifo_try_take_filled();
void              fifo_put_filled(usb_audio_buffer* buffer);


// control / comunicates what kind of data is expected to be filled in the packets
typedef enum
{
	// PCM data coming from the ADC and head switch etc
	fifo_mode_normal,
	// Debug data
	fifo_mode_debug,
}
fifo_mode;

void              fifo_set_mode(fifo_mode mode);
fifo_mode         fifo_get_mode();

#endif

