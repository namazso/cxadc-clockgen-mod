// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include "head_switch.h"

#define HEAD_SWITCH_PIN 16

void head_switch_init()
{
	gpio_init(HEAD_SWITCH_PIN);
	gpio_set_dir(HEAD_SWITCH_PIN, GPIO_IN);
	gpio_pull_down(HEAD_SWITCH_PIN);
}

bool head_switch_sample_pin()
{
	return gpio_get(HEAD_SWITCH_PIN);
}
