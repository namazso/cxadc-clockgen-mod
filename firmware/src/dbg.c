// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Rene Wolf

#include "dbg.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

static uart_inst_t* uart = NULL;
#define not_initialized()  (uart == NULL)

void dbg_init()
{
	if( uart != NULL )
	{
		dbg_say("dbg_init() re-init\n");
		return;
	}
	
	// https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_uart
	// https://github.com/raspberrypi/pico-examples/blob/master/uart/hello_uart/hello_uart.c
	uart = uart0;
	
	// We are using pins 0 and 1, but see the GPIO function select table in the
	// datasheet for information on which other pins can be used.
	uint tx_pin = 0;
	uint rx_pin = 1;
	
	// Set up our UART with the required speed.
	uart_init(uart, 115200);
	
	// Disable CR/LF conversion on UART.
	uart_set_translate_crlf(uart, false);

	// Set the TX and RX pins by using the function select on the GPIO
	// Set datasheet for more information on function select
	gpio_set_function(tx_pin, GPIO_FUNC_UART);
	gpio_set_function(rx_pin, GPIO_FUNC_UART);
	
	dbg_say("dbg_init()\n");
}

static char to_hex(uint32_t n)
{
	n &= 0xf;
	if( n < 10)
		return '0' + n;
	
	n -=10;
	return 'a' + n;
}

static void say_hex(const char* msg)
{
	uart_putc_raw(uart, '0');
	uart_putc_raw(uart, 'x');
	dbg_say(msg);
}

void dbg_u8(uint8_t code)
{
	if( not_initialized() ) return;
	
	char buff[2+1];
	
	buff[2] = 0;
	
	buff[1] = to_hex(code);
	code >>= 4;
	buff[0] = to_hex(code);
	
	say_hex(buff);
}

void dbg_u16(uint16_t code)
{
	if( not_initialized() ) return;
	
	char buff[4+1];
	
	buff[4] = 0;
	
	buff[3] = to_hex(code);
	code >>= 4;
	buff[2] = to_hex(code);
	code >>= 4;
	
	buff[1] = to_hex(code);
	code >>= 4;
	buff[0] = to_hex(code);
	
	say_hex(buff);
}

void dbg_u32(uint32_t code)
{
	if( not_initialized() ) return;
	
	char buff[8+1];
	
	buff[8] = 0;
	
	buff[7] = to_hex(code);
	code >>= 4;
	buff[6] = to_hex(code);
	code >>= 4;
	
	buff[5] = to_hex(code);
	code >>= 4;
	buff[4] = to_hex(code);
	code >>= 4;
	
	buff[3] = to_hex(code);
	code >>= 4;
	buff[2] = to_hex(code);
	code >>= 4;
	
	buff[1] = to_hex(code);
	code >>= 4;
	buff[0] = to_hex(code);
	
	say_hex(buff);
}

void dbg_say(const char* msg)
{
	if( not_initialized() ) return;
	uart_puts(uart, msg);
}

void dbg_dump(const void* data, uint16_t len)
{
	uart_putc_raw(uart, '@');
	dbg_u32((uint32_t)data);
	uart_putc_raw(uart, '[');
	dbg_u16(len);
	dbg_say("]: 0x");

	const uint8_t* data_u8 = data;

	for(int i=0; i<len; ++i)
	{
		uint32_t n = data_u8[i];
		uart_putc_raw(uart, to_hex(n>>4));
		uart_putc_raw(uart, to_hex(n));
	}
}

static void panic_end();

void dbg_panic_code(uint32_t code)
{
	uart_putc_raw(uart, '\n');
	dbg_u32(code);
	uart_putc_raw(uart, '\n');
	dbg_u32(code);
	panic_end();
}

void dbg_panic_msg(const char* msg)
{
	dbg_say(msg);
	panic_end();
}

void dbg_panic_msg_code(const char* msg, uint32_t code)
{
	dbg_say(msg);
	uart_putc_raw(uart, '\n');
	dbg_panic_code(code);
}

static void panic_end()
{
	dbg_say("\n:(\n°_°\nx.X\n");
	while(1) ;
}

#undef not_initialized
