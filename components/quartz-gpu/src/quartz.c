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

#include "quartz.h"
#include <mch2022_badge.h>
#include <pax_internal.h>
#include <driver/gpio.h>

static const char *TAG = "quartz";

extern const uint8_t quartz_bin_start[] asm("_binary_quartz_bin_start");
extern const uint8_t quartz_bin_end[]   asm("_binary_quartz_bin_end");

// For debugging purposes.
void quartz_debug() {
	
}

// Initialise the FPGA GPU system.
void quartz_init() {
	ice40_load_bitstream(get_ice40(), quartz_bin_start, quartz_bin_end - quartz_bin_start);
	vTaskDelay(pdMS_TO_TICKS(5));
	
}

// Select the FPGA's output.
void quartz_select(bool select_fpga_output) {
	ili9341_set_mode(select_fpga_output);
	
	if (select_fpga_output) {
		// Message FPGA about screen custody.
		
	} else {
		// Initialise them screens.
		ili9341_init(get_ili9341());
	}
}
