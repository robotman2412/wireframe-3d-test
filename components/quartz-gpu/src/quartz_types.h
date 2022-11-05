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

#ifndef QUARTZ_TYPES_H
#define QUARTZ_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pax_gfx.h>



// The GPU is currently working on tasks.
#define QUARTZ_STATUS_WORKING	0x0001
// The GPU is in custody of the screen.
#define QUARTZ_STATUS_SCREEN	0x0002
// The GPU is ready to accept more tasks
#define QUARTZ_STATUS_ACCEPTING	0x0004
// This is the first status request since GPU startup.
#define QUARTZ_STATUS_HELLO     0x0008

// A previous command was received incorrectly (invalid checksum or length, etc).
#define QUARTZ_STATUS_ERR_RX	0x0100

// A mask of all error status flags.
#define QUARTZ_ERROR_MASK       0xff00



// A command that can be issued.
typedef enum {
	// Does nothing, and elicits no response.
	QUARTZ_CMD_NOP    = 0,
	// Get version and status.
	// none -> quartz_status_t
	QUARTZ_CMD_STATUS = 0x01,
	// Set the color of the RGB LED.
	// u8 r, u8 g, u8 b -> quartz_status_t
	QUARTZ_CMD_RGBLED = 0x02,
} quartz_cmd_t;



// Common response data for commands.
typedef struct {
	// Whether the data was received correctly.
	bool     rx_valid;
	
	// Architecture ID.
	uint8_t  arch_no;
	// Revision number.
	uint8_t  rev_no;
	// Total capacity for tasks.
	uint16_t task_cap;
	// Amount of tasks in the queue or being worked on.
	uint16_t task_num;
	// Status flags bitfield.
	uint16_t status_flags;
} quartz_status_t;



#ifdef __cplusplus
}
#endif

#endif // QUARTZ_TYPES_H
