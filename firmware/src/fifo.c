// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include <string.h>
#include "fifo.h"
#include "pico/util/queue.h"
#include "pico/critical_section.h"
#include "dbg.h"

static usb_audio_buffer buffers[FIFO_SPACE];

static queue_t pipe_empty;
static queue_t pipe_full;
static critical_section_t mode_mutex;
static fifo_mode mode;

void fifo_init()
{
	queue_init(&pipe_empty, sizeof(usb_audio_buffer*), FIFO_SPACE);
	queue_init(&pipe_full, sizeof(usb_audio_buffer*), FIFO_SPACE);
	critical_section_init(&mode_mutex);
	
	memset(buffers, 0, sizeof(buffers));
	mode = fifo_mode_normal;
	
	for(int i=0; i<FIFO_SPACE; ++i)
	{
		usb_audio_buffer* tmp = &(buffers[i]);
		fifo_put_empty(tmp);
	}

	dbg_say("fifo init with ");
	dbg_u8(FIFO_SPACE);
	dbg_say(" slots in empty\n");
}

usb_audio_buffer* fifo_take_empty()
{
	usb_audio_buffer* ret;
	queue_remove_blocking(&pipe_empty, &ret);
	return ret;
}

usb_audio_buffer* fifo_take_filled()
{
	usb_audio_buffer* ret;
	queue_remove_blocking(&pipe_full, &ret);
	return ret;
}

usb_audio_buffer* fifo_try_take_empty()
{
	usb_audio_buffer* ret;
	if( queue_try_remove(&pipe_empty, &ret) == true )
		return ret;
	return NULL;
}

usb_audio_buffer* fifo_try_take_filled()
{
	usb_audio_buffer* ret;
	if( queue_try_remove(&pipe_full, &ret) == true )
		return ret;
	return NULL;
}

void fifo_put_empty(usb_audio_buffer* buffer)
{
	queue_add_blocking(&pipe_empty, &buffer);
}

void fifo_put_filled(usb_audio_buffer* buffer)
{
	queue_add_blocking(&pipe_full, &buffer);
}

void fifo_set_mode(fifo_mode new_mode)
{
	dbg_say("fifo_set_mode ");
	dbg_say((new_mode == fifo_mode_debug) ? "dbg" : "normal");
	dbg_say("\n");
	
	critical_section_enter_blocking(&mode_mutex);
	mode = new_mode;
	critical_section_exit(&mode_mutex);
}

fifo_mode fifo_get_mode()
{
	critical_section_enter_blocking(&mode_mutex);
	fifo_mode ret = mode;
	critical_section_exit(&mode_mutex);
	return ret;
}
