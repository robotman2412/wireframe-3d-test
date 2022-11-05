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
#include <string.h>

static const char *TAG = "quartz";

extern const uint8_t quartz_bin_start[] asm("_binary_quartz_bin_start");
extern const uint8_t quartz_bin_end[]   asm("_binary_quartz_bin_end");

// Dump some raw bytes.
void dump_bytes(const void *raw, size_t size) {
	const uint8_t *data = raw;
	for (size_t i = 0; i < size; i++) {
		printf("%02x ", data[i]);
	}
}

// For debugging purposes.
void quartz_debug() {
    ESP_LOGI(TAG, "Press A to restart FPGA,");
    ESP_LOGI(TAG, "Press B to get status.");
	
	while (true) {
		quartz_init();
		quartz_select(true);
		gpio_set_direction(GPIO_LCD_RESET, false);
		vTaskDelay(pdMS_TO_TICKS(100));
		gpio_set_direction(GPIO_LCD_RESET, true);
		
		while (true) {
			rp2040_input_message_t msg;
			xQueueReceive(get_rp2040()->queue, &msg, portMAX_DELAY);
			if (msg.input == RP2040_INPUT_BUTTON_ACCEPT && msg.state) {
				break;
			} else if (msg.input == RP2040_INPUT_BUTTON_BACK && msg.state) {
				quartz_status_t status = quartz_cmd_status();
				if (status.rx_valid) {
					ESP_LOGI(TAG, "Status report:");
					ESP_LOGI(TAG, "Arch:     %u Rev %u", status.arch_no, status.rev_no);
					ESP_LOGI(TAG, "Task cap: %u",        status.task_cap);
					ESP_LOGI(TAG, "Task num: %u",        status.task_num);
					ESP_LOGI(TAG, "Status:   %04x",      status.status_flags);
				}
				
			} else if (msg.input == RP2040_INPUT_BUTTON_HOME && msg.state) {
				return;
			}
		}
	}
}



// Initialise the FPGA GPU system.
void quartz_init() {
	if (get_ice40() == NULL) {
		esp_err_t res = bsp_ice40_init();
		if (res) {
			ESP_LOGE(TAG, "GPU init failure (BSP: %s)", esp_err_to_name(res));
		}
	}
	
	// Load the bitstream into the ICE40.
	esp_err_t res = ice40_load_bitstream(get_ice40(), quartz_bin_start, quartz_bin_end - quartz_bin_start);
	if (res) {
		ESP_LOGE(TAG, "GPU init failure (bitstream: %s)", esp_err_to_name(res));
	}
	
	// Give it a bit of time to start up.
	vTaskDelay(pdMS_TO_TICKS(500));
	
	// Initial status check.
	quartz_status_t status = quartz_cmd_status();
	if (status.status_flags & QUARTZ_ERROR_MASK) {
		// Error in initialisation, disable FPGA.
		ice40_disable(get_ice40());
		ESP_LOGE(TAG, "GPU init failure (error code %04x)", status.status_flags);
		
	} else if (~status.status_flags & QUARTZ_STATUS_HELLO) {
		// Error in initialisation, disable FPGA.
		ice40_disable(get_ice40());
		ESP_LOGE(TAG, "GPU init failure (handshake failure)");
		
	} else if (!status.rx_valid) {
		// Error in initialisation, disable FPGA.
		ice40_disable(get_ice40());
		ESP_LOGE(TAG, "GPU init failure (checksum error)");
		
	} else {
		// Successful startup.
		ESP_LOGI(TAG, "GPU init success");
		ESP_LOGI(TAG, "Arch:     %u Rev %u", status.arch_no, status.rev_no);
		ESP_LOGI(TAG, "Task cap: %u",        status.task_cap);
		ESP_LOGI(TAG, "Task num: %u",        status.task_num);
		ESP_LOGI(TAG, "Status:   %04x",      status.status_flags);
	}
}

// Select the FPGA's output.
void quartz_select(bool select_fpga_output) {
	// Reset LCD.
	gpio_set_level(GPIO_LCD_RESET, 0);
	// Set LCD mode.
	gpio_set_level(GPIO_LCD_MODE, select_fpga_output);
	vTaskDelay(5);
	// Release reset line.
	gpio_set_level(GPIO_LCD_RESET, 1);
	
	
	if (select_fpga_output) {
		// Message FPGA about screen custody.
		ESP_LOGI(TAG, "Screen switched to GPU");
		
	} else {
		// Initialise them screens.
		ili9341_init(get_ili9341());
		ESP_LOGI(TAG, "Screen switched to host");
	}
}



// Wait for the FPGA DEVICE to have INCOMING DATA.
// Wait time in milliseconds.
bool quartz_await(uint64_t wait_time) {
	if (!gpio_get_level(GPIO_INT_FPGA)) return true;
	
	if (wait_time) {
		uint64_t limit = esp_timer_get_time() + wait_time * 1000;
		while (esp_timer_get_time() < limit) {
			vTaskDelay(1);
			if (!gpio_get_level(GPIO_INT_FPGA)) return true;
		}
	}
	
	return false;
}

// Compute the quartz checksum over a number of bytes.
uint8_t quartz_checksum(const void *mem, size_t length) {
	const uint8_t *arr = mem;
	uint8_t current = 0xcc;
	for (size_t i = 0; i < length; i++) {
		current = current ^ arr[i];
	}
	return current;
}

// Send a raw quartz command.
// Returns whether the message was successfully received.
bool quartz_cmd_raw1(quartz_cmd_t opcode, uint8_t send, void *send_buf, uint8_t recv, void *recv_buf) {
	// Some temporary buffers for sending and receiving.
	static uint8_t tx_buf[515];
	static uint8_t rx_buf[515];
	
	// Prepare send headers.
	tx_buf[0] = opcode;
	tx_buf[1] = send;
	tx_buf[2] = recv;
	// Insert send data.
	memcpy(&tx_buf[3], send_buf, send);
	memset(&tx_buf[3+send], 0, sizeof(tx_buf) - 3 - send);
	
	// Perform raw SPI transaction.
	esp_err_t res = ice40_transaction(get_ice40(), tx_buf, 4+send+recv, rx_buf, 4+send+recv);
	if (res) {
		ESP_LOGE(TAG, "Comms error: %s", esp_err_to_name(res));
		return false;
	}
	dump_bytes(rx_buf, 4+send+recv);
	
	// Calculate checksum over received.
	uint8_t real_sum = quartz_checksum(&rx_buf[3+send], recv);
	
	// Confirm checksum.
	if (real_sum != rx_buf[3+send+recv]) {
		// Checksum mismatch.
		ESP_LOGE(TAG, "Comms error: Checksum mismatch (host's sum: %02x, GPU's sum: %02x)", real_sum, rx_buf[3+send+recv]);
		return false;
		
	} else {
		// Successfull communication.
		memcpy(recv_buf, &rx_buf[3+send], recv);
		return true;
	}
}

// Send a raw quartz command.
// Returns whether the message was successfully received.
bool quartz_cmd_raw(quartz_cmd_t opcode, uint8_t send, void *send_buf, uint8_t recv, void *recv_buf) {
	// Some temporary buffers for sending and receiving.
	static uint8_t tx_buf[260];
	static uint8_t rx_buf[260];
	
	// Prepare send headers.
	tx_buf[0] = opcode;
	tx_buf[1] = send;
	tx_buf[2] = recv;
	// Insert send data.
	memcpy(&tx_buf[3], send_buf, send);
	
	// Send stuff to the FPGA.
	esp_err_t res = ice40_transaction(get_ice40(), tx_buf, 3+send, rx_buf, 3+send);
	if (res) {
		ESP_LOGE(TAG, "Comms error: %s", esp_err_to_name(res));
		return false;
	}
	printf("\033[32m");
	dump_bytes(tx_buf, 3+send);
	printf("-> ");
	
	// Await receivement time.
	if (!quartz_await(100)) {
		printf("\033[31m<timeout>\033[0m\n");
		ESP_LOGE(TAG, "Comms error: Timeout waiting for response.");
		return false;
	}
	
	// Clear out send data.
	memset(tx_buf, 0, recv+2);
	res = ice40_transaction(get_ice40(), tx_buf, recv+2, rx_buf, recv+2);
	if (res) {
		printf("\033[31m<SPI error>\033[0m\n");
		ESP_LOGE(TAG, "Comms error: %s", esp_err_to_name(res));
		return false;
	}
	
	// Calculate checksum over received.
	uint8_t real_sum = quartz_checksum(rx_buf, 1+recv);
	if (real_sum != rx_buf[1+recv]) {
		printf("\033[31m");
	}
	dump_bytes(rx_buf, 2+recv);
	if (real_sum != rx_buf[1+recv]) {
		printf("(checksum error)");
	}
	printf("\033[0m\n");
	
	// Confirm checksum.
	if (real_sum != rx_buf[1+recv]) {
		// Checksum mismatch.
		ESP_LOGE(TAG, "Comms error: Checksum mismatch (host's sum: %02x, GPU's sum: %02x)", real_sum, rx_buf[1+recv]);
		return false;
		
	} else {
		// Successfull communication.
		memcpy(recv_buf, &rx_buf[1], recv);
		return true;
	}
}


// Send a status request.
quartz_status_t quartz_cmd_status() {
	uint8_t rx[8];
	if (!quartz_cmd_raw(QUARTZ_CMD_STATUS, 0, NULL, sizeof(rx), rx)) {
		return (quartz_status_t) {false};
	} else {
		return (quartz_status_t) {
			.rx_valid     = true,
			
			.arch_no      = rx[0],
			.rev_no       = rx[1],
			.task_cap     = rx[2] | (rx[3] << 8),
			.task_num     = rx[4] | (rx[5] << 8),
			.status_flags = rx[6] | (rx[7] << 8),
		};
	}
}
