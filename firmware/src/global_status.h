// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _GLOBAL_STATUS_H
#define _GLOBAL_STATUS_H

#include <stdint.h>
#include "pico/critical_section.h"

// This will be prefixed on debug output
#define GLOBAL_STATUS_MAGIC_NUMBER 0x11223344


// We use a packed structure so that the memory layout will be exactly as listed here, no additional padding by the compiler included.
// This is importand as we will be later decoding the mem dump on another platform. For the same reason we define our own bool type
// so we know its size.
// https://stackoverflow.com/questions/12213866/is-attribute-packed-ignored-on-a-typedef-declaration
// https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html#index-packed-type-attribute
// https://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe

#define true_u8  1
#define false_u8 0
typedef uint8_t bool_u8;

typedef struct __attribute__((packed))
{
	// true if the general startup of the si5351 clock generator chip looks fine
	bool_u8   si5351_init_success;
	
	// true if there was activity on the PCM1802 lines, updated occasionally
	bool_u8   pcm1802_activity_lrck;
	bool_u8   pcm1802_activity_bck;
	bool_u8   pcm1802_activity_data;
	// some specific counters on the PCM1802 subsystem
	uint32_t  pcm1802_out_of_sync_drops;
	uint32_t  pcm1802_rch_tmo_count;
	uint32_t  pcm1802_rch_tmo_value;
	
	// Counts RX timeout conditions in main1
	uint32_t  main1_rxsample_tmo;
}
global_status_fields;


// This is the global status, to access it use global_status_access({ my code goes here });
extern global_status_fields global_status;
extern critical_section_t global_status_mutex;

void global_status_init();
#define global_status_to_boolu8(v)  (((v) != 0) ? true_u8 : false_u8)

#define global_status_access(n) \
{ \
	critical_section_enter_blocking(&global_status_mutex); \
	n ; \
	critical_section_exit(&global_status_mutex); \
}


#endif

