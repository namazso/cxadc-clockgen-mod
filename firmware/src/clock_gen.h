// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _CLOCK_GEN_H
#define _CLOCK_GEN_H

#include <stdint.h>
#include <stdbool.h>

#define CLOCK_GEN_CXADC_CLOCK_F0_STR  "20MHz"
#define CLOCK_GEN_CXADC_CLOCK_F1_STR  "28.63MHz"
#define CLOCK_GEN_CXADC_CLOCK_F2_STR  "40MHz"
#define CLOCK_GEN_CXADC_CLOCK_F3_STR  "50MHz"


bool clock_gen_init();
void clock_gen_default();

const uint32_t* clock_gen_get_adc_sample_rate_options(uint8_t* len);
uint32_t        clock_gen_get_adc_sample_rate();
void            clock_gen_set_adc_sample_rate(uint32_t rate_hz);

uint8_t clock_gen_get_cxadc_sample_rate(uint8_t output);
void    clock_gen_set_cxadc_sample_rate(uint8_t output, uint8_t frequency_option);


#endif

