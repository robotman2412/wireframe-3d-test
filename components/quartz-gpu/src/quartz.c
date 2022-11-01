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
#include <hardware.h>
#include <pax_internal.h>
#include <driver/gpio.h>

static const char *TAG = "quartz";

extern const uint8_t quartz_bin_start[] asm("_binary_quartz_bin_start");
extern const uint8_t quartz_bin_end[]   asm("_binary_quartz_bin_end");

// Dump some raw bytes.
void dump_bytes(const void *raw, size_t size) {
	const uint8_t *data = raw;
	for (size_t i = 0; i < size; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n");
}

// For debugging purposes.
void quartz_debug() {
	
}



// Initialise the FPGA GPU system.
void quartz_init() {
	if (get_ice40() == NULL) {
		esp_err_t res = bsp_ice40_init();
		if (res) {
			ESP_LOGE(TAG, "GPU init failure (BSP)");
		}
	}
	
	// Load the bitstream into the ICE40.
	ice40_load_bitstream(get_ice40(), quartz_bin_start, quartz_bin_end - quartz_bin_start);
	// Give it a bit of time to start up.
	vTaskDelay(pdMS_TO_TICKS(5));
	
	// Initial status check.
	quartz_status_t status = quartz_cmd_status();
	if (!status.rx_valid || !(status.status_flags & QUARTZ_STATUS_HELLO)) {
		// Error in initialisation, disable FPGA.
		ice40_disable(get_ice40());
		ESP_LOGE(TAG, "GPU init failure (quartz)");
		
	} else {
		// Successful startup.
		ESP_LOGI(TAG, "GPU init success");
	}
	
	if (status.rx_valid) {
		ESP_LOGI(TAG, "Arch:     %u Rev %u", status.arch_no, status.rev_no);
		ESP_LOGI(TAG, "Task cap: %u", status.task_cap);
		ESP_LOGI(TAG, "Status:   %04x", status.status_flags);
	}
}

// Select the FPGA's output.
void quartz_select(bool select_fpga_output) {
	gpio_set_direction(GPIO_LCD_MODE, select_fpga_output);
	
	if (select_fpga_output) {
		// Message FPGA about screen custody.
		
	} else {
		// Initialise them screens.
		ili9341_init(get_ili9341());
	}
}



quartz_status_t quartz_cmd_status() {
	uint8_t tx[3 + 6] = {
		// Send status, 0 tx bytes, 6 rx bytes
		QUARTZ_CMD_STATUS, 0, 6
	};
	uint8_t rx[3 + 6];
	esp_err_t res = ice40_transaction(get_ice40(), tx, sizeof(tx), rx, sizeof(rx));
	dump_bytes(rx, sizeof(rx));
	if (res) {
		return (quartz_status_t) {false};
	} else {
		return (quartz_status_t) {
			.rx_valid     = true,
			
			.arch_no      = rx[3 + 0],
			.rev_no       = rx[3 + 1],
			.task_cap     = rx[3 + 2] | (rx[3 + 3] << 8),
			.status_flags = rx[3 + 4] | (rx[3 + 5] << 8),
		};
	}
}
