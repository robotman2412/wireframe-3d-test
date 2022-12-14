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


#include "main.h"
#include "wf3d.h"
#include "quartz.h"

static pax_buf_t buf;
xQueueHandle buttonQueue;

#include <esp_log.h>
static const char *TAG = "main";

extern const char suzanne_obj_start[] asm("_binary_suzanne_obj_start");
extern const char suzanne_obj_end[]   asm("_binary_suzanne_obj_end");

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
  
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "Starting...");

    // Initialize the screen, the I2C and the SPI busses.
    bsp_init();

    // Initialize the RP2040 (responsible for buttons, etc).
    bsp_rp2040_init();
    
    // This queue is used to receive button presses.
    buttonQueue = get_rp2040()->queue;
    
    // Initialize graphics for the screen.
    pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);
    pax_background(&buf, 0xff000000);
    pax_enable_multicore(1);
    // quartz_init();
    // quartz_debug();
    // exit_to_launcher();
    
    // Initialize NVS.
    nvs_flash_init();
    
    // Initialize WiFi. This doesn't connect to Wifi yet.
    wifi_init();
        
    wf3d_ctx_t c3d;
    wf3d_init(&c3d);
    c3d.depth = malloc(sizeof(depth_t) * buf.width * buf.height);
    
    bool up = 0, down = 0, left = 0, right = 0;
    
    FILE *fd = fmemopen((void *) suzanne_obj_start, suzanne_obj_end - suzanne_obj_start, "r");
    wf3d_shape_t *suzanne = s3d_decode_obj(fd);
    wf3d_shape_t *sphere  = s3d_uv_sphere((vec3f_t){0, 0, 0}, 1.5, 5, 10);
    
    int mode = 0;
    int scene = 1;
    float eye_dist = 0.18;
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
        
        // Move around a bit.
        wf3d_apply_3d(&c3d, matrix_3d_translate(0, 0, 2));
        wf3d_apply_3d(&c3d, matrix_3d_scale(1.2, 1.2, 1.2));
        float a = esp_timer_get_time() % 30000000 / 10000000.0 * 2 * M_PI;
        wf3d_apply_3d(&c3d, matrix_3d_rotate_y(a));
        
        // // Sphere.
        // wf3d_push_3d (&c3d);
        // wf3d_apply_3d(&c3d, matrix_3d_translate(1, 1, 1));
        // wf3d_apply_3d(&c3d, matrix_3d_scale(0.25, 0.25, 0.25));
        // wf3d_apply_3d(&c3d, matrix_3d_rotate_z(a/3));
        // wf3d_mesh    (&c3d, sphere);
        // wf3d_pop_3d  (&c3d);
        
        // // Sphere 2!
        // wf3d_push_3d (&c3d);
        // wf3d_apply_3d(&c3d, matrix_3d_translate(-1, -1, -1));
        // wf3d_apply_3d(&c3d, matrix_3d_scale(0.25, 0.25, 0.25));
        // wf3d_apply_3d(&c3d, matrix_3d_rotate_z(a/3));
        // wf3d_mesh    (&c3d, sphere);
        // wf3d_pop_3d  (&c3d);
        
        // Add the shapes.
        if (suzanne && scene == 1) {
            wf3d_mesh(&c3d, suzanne);
        } else {
            wf3d_line(&c3d, (vec3f_t) {-1, -1, 0}, (vec3f_t) {1, 1, 0});
            wf3d_lines(&c3d, 8, cube_vtx, 12, cube_lines);
        }
        
        if (left && !right) {
            eye_dist /= 1.05;
        } else if (right && !left) {
            eye_dist *= 1.05;
        }
        
        // Make a camera matrix.
        matrix_3d_t cam_mtx = 
            matrix_3d_identity();
            // matrix_3d_multiply(matrix_3d_rotate_y(a), matrix_3d_translate(0, 0, 2));
            // matrix_3d_multiply(matrix_3d_translate(0, 0, 2), matrix_3d_rotate_y(a));
            // matrix_3d_rotate_y(a);
            // matrix_3d_translate(0, 0, 2);
        
        // Render 3D stuff.
        if (mode == 0) wf3d_render (&buf, 0xffafafaf, &c3d, cam_mtx); // Regular projected 3D.
        if (mode == 1) wf3d_render2(&buf, 0xffff0000, 0xff00ffff, &c3d, cam_mtx, eye_dist); // Red, Cyan
        if (mode == 2) wf3d_render2(&buf, 0xffffff00, 0xff0000ff, &c3d, cam_mtx, eye_dist); // Yellow, Blue
        wf3d_clear(&c3d);
        
        // Draws the entire graphics buffer to the screen.
        disp_flush();
        
        // Structure used to receive data.
        rp2040_input_message_t message;
        
        // Wait for button presses and do another cycle.
        if (xQueueReceive(buttonQueue, &message, 0)) {
            // Which button is currently pressed?
            if (message.input == RP2040_INPUT_BUTTON_HOME && message.state) {
                // If home is pressed, exit to launcher.
                exit_to_launcher();
            } else if (message.input == RP2040_INPUT_BUTTON_ACCEPT && message.state) {
                // Cycle render mode.
                mode ++;
                mode %= 3;
            } else if (message.input == RP2040_INPUT_BUTTON_BACK && message.state) {
                // Cycle render mode.
                scene ++;
                scene %= 2;
            } else if (message.input == RP2040_INPUT_JOYSTICK_UP) {
                up    = message.state;
            } else if (message.input == RP2040_INPUT_JOYSTICK_DOWN) {
                down  = message.state;
            } else if (message.input == RP2040_INPUT_JOYSTICK_LEFT) {
                left  = message.state;
            } else if (message.input == RP2040_INPUT_JOYSTICK_RIGHT) {
                right = message.state;
            }
        }
    }
}
