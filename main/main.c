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
    
    bool up = 0, down = 0, left = 0, right = 0;
    
    wf3d_shape_t *sphere = s3d_uv_sphere((vec3f_t){0, 0, 0}, 1, 5, 10);
    
    int mode = 1;
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
        float a = esp_timer_get_time() % 30000000 / 10000000.0 * 2 * M_PI;
        wf3d_apply_3d(&c3d, matrix_3d_rotate_y(a));
        
        // Sphere.
        wf3d_push_3d (&c3d);
        wf3d_apply_3d(&c3d, matrix_3d_translate(1, 1, 1));
        wf3d_apply_3d(&c3d, matrix_3d_scale(0.25, 0.25, 0.25));
        wf3d_apply_3d(&c3d, matrix_3d_rotate_z(a/3));
        wf3d_lines   (&c3d, sphere->num_vertex, sphere->vertices, sphere->num_lines, sphere->indices);
        wf3d_pop_3d  (&c3d);
        
        // Sphere 2!
        wf3d_push_3d (&c3d);
        wf3d_apply_3d(&c3d, matrix_3d_translate(-1, -1, -1));
        wf3d_apply_3d(&c3d, matrix_3d_scale(0.25, 0.25, 0.25));
        wf3d_apply_3d(&c3d, matrix_3d_rotate_z(a/3));
        wf3d_lines   (&c3d, sphere->num_vertex, sphere->vertices, sphere->num_lines, sphere->indices);
        wf3d_pop_3d  (&c3d);
        
        // Add the shapes.
        wf3d_line(&c3d, (vec3f_t) {-1, -1, 0}, (vec3f_t) {1, 1, 0});
        wf3d_lines(&c3d, 8, cube_vtx, 12, cube_lines);
        
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
        if (mode == 1) wf3d_render2(&buf, 0xffff0000, 0xff00ffff, &c3d, cam_mtx, eye_dist); // Red, Cyan
        if (mode == 2) wf3d_render2(&buf, 0xffff7f00, 0xff0000ff, &c3d, cam_mtx, eye_dist); // Orange, Blue
        if (mode == 0) wf3d_render(&buf, 0xffffffff, &c3d, cam_mtx); // Regular projected 3D.
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
