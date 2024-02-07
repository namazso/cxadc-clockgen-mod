// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#ifndef _DBG_H
#define _DBG_H

#include <stdint.h>

void dbg_init();

void dbg_u8(uint8_t code);
void dbg_u16(uint16_t code);
void dbg_u32(uint32_t code);

void dbg_say(const char* msg);
void dbg_dump(const void* data, uint16_t len);

void dbg_panic_code(uint32_t code);
void dbg_panic_msg(const char* msg);
void dbg_panic_msg_code(const char* msg, uint32_t code);

#endif

