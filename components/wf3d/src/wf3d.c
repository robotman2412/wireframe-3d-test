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

#include "wf3d.h"
#include <math.h>
#include <string.h>
#include <esp_log.h>



static const char *TAG = "wf-3d";

static void dump_ctx(wf3d_ctx_t *ctx) {
	ESP_LOGI(TAG, "==== CONTEXT DUMP ====");
	ESP_LOGI(TAG, "Vertices:");
	for (size_t i = 0; i < ctx->num_vertex; i++) {
		ESP_LOGI(TAG, "  (%3f, %3f, %3f)", ctx->vertices[i].x, ctx->vertices[i].y, ctx->vertices[i].z);
	}
	ESP_LOGI(TAG, "Lines:");
	for (size_t i = 0; i < ctx->num_line; i++) {
		ESP_LOGI(TAG, "  %d -> %d", ctx->lines[i*2], ctx->lines[i*2+1]);
	}
}



// MAKEs a new DRAWING QUEUE.
void wf3d_init(wf3d_ctx_t *ctx) {
	*ctx = (wf3d_ctx_t) {
		.num_line     = 0,
		.cap_line     = WF3D_INITIAL_LINE_CAP,
		.lines        = malloc(2 * sizeof(size_t) * WF3D_INITIAL_LINE_CAP),
		.xfrom_vertex = 0,
		.num_vertex   = 0,
		.cap_vertex   = WF3D_INITIAL_VERTEX_CAP,
		.vertices     = malloc(sizeof(vec3f_t) * WF3D_INITIAL_VERTEX_CAP),
		.cam_mode     = CAMERA_VERTICAL_FOV,
		.cam_var      = 30,
		.stack        = {
			.parent = NULL,
			.value  = matrix_3d_identity(),
		}
	};
}

// DESTROYs a DRAWING QUEUE.
void wf3d_destroy(wf3d_ctx_t *ctx) {
	free(ctx->lines);
	free(ctx->vertices);
	wf3d_reset_3d(ctx);
}

// CLEARs the DRAWING QUEUE.
void wf3d_clear(wf3d_ctx_t *ctx) {
	ctx->num_line     = 0;
	ctx->num_vertex   = 0;
	ctx->xfrom_vertex = 0;
	wf3d_reset_3d(ctx);
}



// An ADDITIVE shading device.
pax_col_t wf3d_shader_cb_additive(pax_col_t tint, pax_col_t existing, int x, int y, float u, float v, void *args) {
	uint16_t r = (existing >> 16) & 255;
	uint16_t g = (existing >>  8) & 255;
	uint16_t b =  existing        & 255;
	r += (tint >> 16) & 255;
	g += (tint >>  8) & 255;
	b +=  tint        & 255;
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	return pax_col_rgb(r, g, b);
}

const pax_shader_t wf3d_shader_additive = {
	.schema_version    =  1,
	.schema_complement = ~1,
	.renderer_id       = PAX_RENDERER_ID_SWR,
	.promise_callback  = NULL,
	.callback          = wf3d_shader_cb_additive,
	.callback_args     = NULL,
	.alpha_promise_0   = true,
	.alpha_promise_255 = true,
};

// Adds a line to the DRAWING QUEUE.
void wf3d_line(wf3d_ctx_t *ctx, vec3f_t start, vec3f_t end) {
	vec3f_t verts[]   = {start, end};
	size_t  indices[] = {0, 1};
	wf3d_lines(ctx, 2, verts, 1, indices);
}

// Adds multiple LINEs to the DRAWING QUEUE.
void wf3d_lines(wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_lines, size_t *indices) {
	// Ensure array space for VTX.
	if (ctx->cap_vertex <= ctx->num_vertex + num_vertices) {
		while (ctx->cap_vertex <= ctx->num_vertex + num_vertices) {
			ctx->cap_vertex = ctx->cap_vertex * 3 / 2;
		}
		ctx->vertices = realloc(ctx->vertices, sizeof(vec3f_t) * ctx->cap_vertex);
	}
	
	// Ensure array space for LN.
	if (ctx->cap_line <= ctx->num_line + num_lines) {
		while (ctx->cap_line <= ctx->num_line + num_lines) {
			ctx->cap_line = ctx->cap_line * 3 / 2;
		}
		ctx->lines = realloc(ctx->lines, 2 * sizeof(vec3f_t) * ctx->cap_line);
	}
	
	// Insert VTX.
	for (size_t i = 0; i < num_vertices; i++) {
		vec3f_t *ptr = &ctx->vertices[ctx->num_vertex + i];
		*ptr = vertices[i];
		matrix_3d_transform(ctx->stack.value, &ptr->x, &ptr->y, &ptr->z);
	}
	
	// Insert LINE.
	for (size_t i = 0; i < num_lines; i++) {
		ctx->lines[2*(i+ctx->num_line)]   = indices[i*2]   + ctx->num_vertex;
		ctx->lines[2*(i+ctx->num_line)+1] = indices[i*2+1] + ctx->num_vertex;
	}
	
	ctx->num_vertex += num_vertices;
	ctx->num_line += num_lines;
}

// DRAWs everything in the DRAWING QUEUE.
void wf3d_render(pax_buf_t *to, pax_col_t color, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix) {
	// Get camera information.
	float focal = wf3d_get_foc(to, ctx);
	
	// Transform 3D points into 2D.
	vec3f_t *xform_vtx = malloc(sizeof(vec3f_t) * ctx->num_vertex);
	for (size_t i = 0; i < ctx->num_vertex; i++) {
		vec3f_t raw_vtx = ctx->vertices[i];
		matrix_3d_transform(cam_matrix, &raw_vtx.x, &raw_vtx.y, &raw_vtx.z);
		xform_vtx[i] = wf3d_xform(ctx, focal, raw_vtx);
	}
	
	// Set up PAX transform thingy.
	pax_push_2d(to);
	float scale = fminf(to->width, to->height);
	pax_apply_2d(to, matrix_2d_translate(to->width / 2.0, to->height / 2.0));
	pax_apply_2d(to, matrix_2d_scale(scale / 2, scale / 2));
	
	// Draw lines.
	for (size_t i = 0; i < ctx->num_line; i++) {
		size_t start_idx = ctx->lines[2*i];
		size_t end_idx   = ctx->lines[2*i + 1];
		if (start_idx >= ctx->num_vertex || end_idx >= ctx->num_vertex) continue;
		
		if (xform_vtx[start_idx].z >= 0 && xform_vtx[end_idx].z >= 0) {
			pax_shade_line(
				to, color,
				&wf3d_shader_additive,
				xform_vtx[start_idx].x, xform_vtx[start_idx].y,
				xform_vtx[end_idx].x, xform_vtx[end_idx].y
			);
		}
	}
	
	// Clean up.
	pax_pop_2d(to);
	free(xform_vtx);
}

// DRAWs everything in one color per eye.
void wf3d_render2(pax_buf_t *to, pax_col_t left_eye, pax_col_t right_eye, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix, float eye_dist) {
	wf3d_render(to, left_eye, ctx, matrix_3d_multiply(matrix_3d_translate(-eye_dist/2, 0, 0), cam_matrix));
	wf3d_render(to, right_eye, ctx, matrix_3d_multiply(matrix_3d_translate(eye_dist/2, 0, 0), cam_matrix));
}



// Determines the focal depth to use in the given context.
float wf3d_get_foc(pax_buf_t *buf, wf3d_ctx_t *ctx) {
	float scale = fminf(buf->width, buf->height);
	switch (ctx->cam_mode) {
		default:
			return 1;
			
		case CAMERA_FOCAL_DEPTH:
			return ctx->cam_var;
			
		case CAMERA_VERTICAL_FOV:
			return buf->height / scale / 2.0 / tanf(ctx->cam_var / 360.0 * M_PI);
			
		case CAMERA_HORIZONTAL_FOV:
			return buf->width  / scale / 2.0 / tanf(ctx->cam_var / 360.0 * M_PI);
	}
}

// Transforms a 3D point into 2D.
vec3f_t wf3d_xform(wf3d_ctx_t *ctx, float focal_depth, vec3f_t point) {
	return (vec3f_t) {
		point.x * (focal_depth / (focal_depth + point.z)),
		point.y * (focal_depth / (focal_depth + point.z)),
		point.z,
	};
}

// APPLY some MATRIX.
void wf3d_apply_3d(wf3d_ctx_t *ctx, matrix_3d_t mtx) {
	ctx->stack.value = matrix_3d_multiply(ctx->stack.value, mtx);
}

// PUSH to the MATRIX STACK to SAVE FOR LATER.
void wf3d_push_3d(wf3d_ctx_t *ctx) {
	matrix_stack_3d_t *parent = malloc(sizeof(matrix_stack_3d_t));
	if (!parent) return;
	*parent = ctx->stack;
	ctx->stack.parent = parent;
}

// POP from the MATRIX STACK to RESTORE.
void wf3d_pop_3d(wf3d_ctx_t *ctx) {
	matrix_stack_3d_t *parent = ctx->stack.parent;
	if (!parent) return;
	ctx->stack = *parent;
	free(parent);
}

// RESET the MATRIX STACK.
void wf3d_reset_3d(wf3d_ctx_t *ctx) {
	while (ctx->stack.parent) {
		matrix_stack_3d_t *parent = ctx->stack.parent;
		ctx->stack = *parent;
		free(parent);
	}
	ctx->stack.value = matrix_3d_identity();
}



// Creates a UV sphere mesh.
wf3d_shape_t *s3d_uv_sphere(vec3f_t position, float radius, int latitude_cuts, int longitude_cuts) {
	// Sanity checks.
	if (latitude_cuts < 2 || longitude_cuts < 2) return NULL;
	
	// Latitude cuts:  Adds vertices up / down.
	// Longitude cuts: Adds vertices left / right.
	
	// Determine line counts.
	size_t num_vertex = 2 + latitude_cuts * longitude_cuts;
	size_t num_line   = longitude_cuts * (latitude_cuts * 2 + 1);
	
	// Allocate memory.
	size_t vertices_size = num_vertex * sizeof(vec3f_t);
	size_t lines_size  = num_line * sizeof(size_t) * 2;
	size_t memory      = (size_t) malloc(sizeof(wf3d_shape_t) + vertices_size + lines_size);
	wf3d_shape_t *shape    = (void *)  memory;
	vec3f_t      *vertices = (void *) (memory + sizeof(wf3d_shape_t));
	size_t       *indices  = (void *) (memory + sizeof(wf3d_shape_t) + vertices_size);
	
	// Generate vertices.
	vertices[0] = (vec3f_t) {0,  1, 0};
	vertices[1] = (vec3f_t) {0, -1, 0};
	
	vec3f_t     vtx    = {0, 1, 0};
	matrix_3d_t lat_rot_mtx = matrix_3d_rotate_z(M_PI / (latitude_cuts + 1));
	for (size_t i = 0; i < latitude_cuts; i++) {
		vtx = matrix_3d_transform_inline(lat_rot_mtx, vtx);
		for (size_t x = 0; x < longitude_cuts; x++) {
			vertices[2 + i + x * latitude_cuts] = vtx;
		}
	}
	
	// Translate vertices.
	matrix_3d_t lon_rot_mtx = matrix_3d_rotate_y(M_PI * 2.0 / longitude_cuts);
	matrix_3d_t cur_mtx     = (matrix_3d_t) { .arr = {
		radius, 0,      0,      position.x,
		0,      radius, 0,      position.y,
		0,      0,      radius, position.z,
	}};
	for (size_t i = 0; i < longitude_cuts; i++) {
		for (size_t x = 0; x < latitude_cuts; x++) {
			vertices[2 + x + i * latitude_cuts] = matrix_3d_transform_inline(cur_mtx, vertices[2 + x + i * latitude_cuts]);
		}
		
		cur_mtx = matrix_3d_multiply(cur_mtx, lon_rot_mtx);
	}
	
	// Generate indices.
	size_t line_lon_offset = 2 + latitude_cuts * 4;
	for (size_t lon = 0; lon < longitude_cuts; lon++) {
		size_t line_idx   = line_lon_offset * lon;
		size_t vertex_idx = 2 + lon * latitude_cuts;
		indices[line_idx]     = 0;
		indices[line_idx + 1] = vertex_idx;
		indices[line_idx + 2] = 1;
		indices[line_idx + 3] = vertex_idx + latitude_cuts - 1;
		indices[line_idx + 4] = vertex_idx;
		indices[line_idx + 5] = 2 + (lon * latitude_cuts + latitude_cuts) % (num_vertex - 2);
		line_idx += 6;
		for (size_t lat = 0; lat < latitude_cuts - 1; lat++) {
			indices[line_idx + lat * 4]     = lat + vertex_idx;
			indices[line_idx + lat * 4 + 1] = lat + vertex_idx + 1;
			indices[line_idx + lat * 4 + 2] = lat + vertex_idx + 1;
			indices[line_idx + lat * 4 + 3] = 2 + (lon * latitude_cuts + lat + latitude_cuts + 1) % (num_vertex - 2);
		}
	}
	
	// Fill in the shape.
	shape->num_vertex = num_vertex;
	shape->vertices   = vertices;
	shape->num_lines  = num_line;
	shape->indices    = indices;
	return shape;
}
