// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _PCM1802_H
#define _PCM1802_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "pico/stdlib.h"

extern uint32_t pcm1802_out_of_sync_drops;
extern uint32_t pcm1802_rch_tmo_count;
extern uint32_t pcm1802_rch_tmo_value;

void pcm1802_init();
void pcm1802_power_up();
void pcm1802_power_down();
// Blocking     receive of one sample on L+R channels in USB UAC PCM Type I format
void pcm1802_rx_24bit_uac_pcm_type1(uint8_t* l_3byte, uint8_t* r_3byte);
// Non-blocking receive of one sample on L+R channels in USB UAC PCM Type I format, returns true if successful
bool pcm1802_try_rx_24bit_uac_pcm_type1(uint8_t* l_3byte, uint8_t* r_3byte);

// Checks for activity on pins, this can be used to debug problems.
// Each function may also use some additional logic to filter out unwanted behavior, 
// improving the detection quality. Callin one of these may take a couple ms to complete.
bool pcm1802_activity_on_lrck();
bool pcm1802_activity_on_bck();
bool pcm1802_activity_on_data();

#ifdef __cplusplus
}
#endif

#endif /* _PCM1802_H */
