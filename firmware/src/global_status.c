// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include <string.h>
#include "global_status.h"

#include "pico/stdlib.h"

critical_section_t global_status_mutex;
global_status_fields global_status;

void global_status_init()
{
	critical_section_init(&global_status_mutex);
	memset(&global_status, 0, sizeof(global_status));
}
