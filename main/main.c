/*
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

// This file contains a simple Hello World app which you can base you own
// native Badge apps on.

#include "main.h"
#include "wf3d.h"

static pax_buf_t buf;
xQueueHandle buttonQueue;

#include <esp_log.h>
static const char *TAG = "mch2022-demo-app";

// Updates the screen with the latest buffer.
void disp_flush() {
    ili9341_write(get_ili9341(), buf.buf);
}

// Exits the app, returning to the launcher.
void exit_to_launcher() {
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    esp_restart();
}

void app_main() {
  
    
    ESP_LOGI(TAG, "Welcome to the template app!");

    // Initialize the screen, the I2C and the SPI busses.
    bsp_init();

    // Initialize the RP2040 (responsible for buttons, etc).
    bsp_rp2040_init();
    
    // This queue is used to receive button presses.
    buttonQueue = get_rp2040()->queue;
    
    // Initialize graphics for the screen.
    pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);
    pax_background(&buf, 0xff000000);
    
    // Initialize NVS.
    nvs_flash_init();
    
    // Initialize WiFi. This doesn't connect to Wifi yet.
    wifi_init();
        
    wf3d_ctx_t c3d;
    wf3d_init(&c3d);
    
    while (1) {
        pax_background(&buf, 0);
        
        // 3D unit CUBE test.
        vec3f_t cube_vtx[] = {
            // Front face
            { -1, -1, -1 },
            {  1, -1, -1 },
            {  1,  1, -1 },
            { -1,  1, -1 },
            
            // Back face
            { -1, -1,  1 },
            {  1, -1,  1 },
            {  1,  1,  1 },
            { -1,  1,  1 },
        };
        
        size_t cube_lines[] = {
            // Front face
            0, 1,
            1, 2,
            2, 3,
            3, 0,
            
            // Back face
            4, 5,
            5, 6,
            6, 7,
            7, 4,
            
            // Edge faces
            0, 4,
            1, 5,
            2, 6,
            3, 7,
        };
        
        // Add the cube.
        wf3d_lines(&c3d, 8, cube_vtx, 12, cube_lines);
        wf3d_line(&c3d, (vec3f_t) {-1, -1, 0}, (vec3f_t) {1, 1, 0});
        
        // Make a camera matrix.
        float a = esp_timer_get_time() % 10000000 / 10000000.0 * 2 * M_PI;
        matrix_3d_t cam_mtx = 
            // matrix_3d_multiply(matrix_3d_rotate_y(a), matrix_3d_translate(0, 0, 2));
            matrix_3d_multiply(matrix_3d_translate(0, 0, 2), matrix_3d_rotate_y(a));
            // matrix_3d_rotate_y(a);
            // matrix_3d_translate(0, 0, 2);
        
        // Render 3D stuff.
        wf3d_render2(&buf, 0xff00ffff, 0xffff0000, &c3d, cam_mtx);
        wf3d_clear(&c3d);
        
        // Draws the entire graphics buffer to the screen.
        disp_flush();
        
        // Wait for button presses and do another cycle.
        
        // Structure used to receive data.
        rp2040_input_message_t message;
        
        if (xQueueReceive(buttonQueue, &message, 0)) {
            // Which button is currently pressed?
            if (message.input == RP2040_INPUT_BUTTON_HOME && message.state) {
                // If home is pressed, exit to launcher.
                exit_to_launcher();
            }
        }
    }
}
