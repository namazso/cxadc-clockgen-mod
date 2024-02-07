// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _HEAD_SWITCH_H
#define _HEAD_SWITCH_H

#include <stdint.h>
#include "pico/stdlib.h"

void head_switch_init();
bool head_switch_sample_pin();

#endif

