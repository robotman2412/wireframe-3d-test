/*
	MIT License

	Copyright (c) 2022 Julian Scheffers

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#ifndef QUARTZ_H
#define QUARTZ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "quartz_types.h"

#include <pax_gfx.h>
#include <ice40.h>
#include <ili9341.h>

// For debugging purposes.
void quartz_debug();

// Initialise the FPGA GPU system.
void quartz_init();

// Compute the quartz checksum over a number of bytes.
uint8_t quartz_checksum(const void *mem, size_t length);
// Send a raw quartz command.
// Returns whether the message was successfully received.
bool    quartz_cmd_raw (quartz_cmd_t opcode, uint8_t send, void *send_buf, uint8_t recv, void *recv_buf);

// Send a status request.
quartz_status_t quartz_cmd_status();

#ifdef __cplusplus
}
#endif

#endif // QUARTZ_H
