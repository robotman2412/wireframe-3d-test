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
		.num_tri      = 0,
		.cap_tri      = WF3D_INITIAL_TRI_CAP,
		.tris         = malloc(3 * sizeof(size_t) * WF3D_INITIAL_TRI_CAP),
		.num_vertex   = 0,
		.cap_vertex   = WF3D_INITIAL_VERTEX_CAP,
		.vertices     = malloc(sizeof(vec3f_t) * WF3D_INITIAL_VERTEX_CAP),
		.cam_mode     = CAMERA_VERTICAL_FOV,
		.cam_var      = 60,
		.stack        = {
			.parent = NULL,
			.value  = matrix_3d_identity(),
		}
	};
}

// DESTROYs a DRAWING QUEUE.
void wf3d_destroy(wf3d_ctx_t *ctx) {
	free(ctx->lines);
	free(ctx->tris);
	free(ctx->vertices);
	wf3d_reset_3d(ctx);
}

// CLEARs the DRAWING QUEUE.
void wf3d_clear(wf3d_ctx_t *ctx) {
	ctx->num_line     = 0;
	ctx->num_tri      = 0;
	ctx->num_vertex   = 0;
	wf3d_reset_3d(ctx);
}



static inline depth_t float_to_depth(float in, float max) {
	return UINT16_MAX * (in / max);
}

// A DEPTH BUFFER shading device.
pax_col_t wf3d_shader_cb_depth(pax_col_t tint, pax_col_t existing, int x, int y, float u, float v, void *args) {
	wf3d_ctx_t *ctx = args;
	
	if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
		return existing;
	}
	
	depth_t depth = u;
	depth_t existing_depth = ctx->depth[x + y*ctx->width];
	
	if (depth < existing_depth) {
		ctx->depth[x + y*ctx->width] = depth;
		return 0xff000000 | (tint & ctx->mask) | (existing & ~ctx->mask);
	} else {
		return existing;
	}
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

// A MAXIMUM shading device.
pax_col_t wf3d_shader_cb_maximum(pax_col_t tint, pax_col_t existing, int x, int y, float u, float v, void *args) {
	uint16_t r0 = (existing >> 16) & 255;
	uint16_t g0 = (existing >>  8) & 255;
	uint16_t b0 =  existing        & 255;
	uint16_t r1 = (tint >> 16) & 255;
	uint16_t g1 = (tint >>  8) & 255;
	uint16_t b1 =  tint        & 255;
	return pax_col_rgb(
		r0 > r1 ? r0 : r1,
		g0 > g1 ? g0 : g1,
		b0 > b1 ? b0 : b1
	);
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

const pax_shader_t wf3d_shader_maximum = {
	.schema_version    =  1,
	.schema_complement = ~1,
	.renderer_id       = PAX_RENDERER_ID_SWR,
	.promise_callback  = NULL,
	.callback          = wf3d_shader_cb_maximum,
	.callback_args     = NULL,
	.alpha_promise_0   = true,
	.alpha_promise_255 = true,
};

// Adds a line to the DRAWING QUEUE.
void wf3d_line(wf3d_ctx_t *ctx, vec3f_t start, vec3f_t end) {
	vec3f_t verts[]        = {start, end};
	size_t  line_indices[] = {0, 1};
	wf3d_add(ctx, 2, verts, 1, line_indices, 0, NULL);
}

// Adds multiple LINEs to the DRAWING QUEUE.
void wf3d_lines(wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_lines, size_t *line_indices) {
	wf3d_add(ctx, num_vertices, vertices, num_lines, line_indices, 0, NULL);
}

// Adds a triangle to the DRAWING QUEUE.
void wf3d_tri(wf3d_ctx_t *ctx, vec3f_t a, vec3f_t b, vec3f_t c) {
	vec3f_t verts[]       = {a, b, c};
	size_t  tri_indices[] = {0, 1, 2};
	wf3d_add(ctx, 2, verts, 0, NULL, 1, tri_indices);
}

// Adds multiple TRIANGLES to the DRAWING QUEUE.
void wf3d_tris(wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_tris, size_t *tri_indices) {
	wf3d_add(ctx, num_vertices, vertices, 0, NULL, num_tris, tri_indices);
}

// Adds multiple LINEs and TRIANGLEs to the DRAWING QUEUE.
void wf3d_add(wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_lines, size_t *line_indices, size_t num_tris, size_t *tri_indices) {
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
		ctx->lines = realloc(ctx->lines, 2 * sizeof(size_t) * ctx->cap_line);
	}
	
	// Ensure array space for TRI.
	if (ctx->cap_tri <= ctx->num_tri + num_tris) {
		while (ctx->cap_tri <= ctx->num_tri + num_tris) {
			ctx->cap_tri = ctx->cap_tri * 3 / 2;
		}
		ctx->tris = realloc(ctx->tris, 3 * sizeof(size_t) * ctx->cap_tri);
	}
	
	// Insert VTX.
	for (size_t i = 0; i < num_vertices; i++) {
		vec3f_t *ptr = &ctx->vertices[ctx->num_vertex + i];
		*ptr = vertices[i];
		matrix_3d_transform(ctx->stack.value, &ptr->x, &ptr->y, &ptr->z);
	}
	
	// Insert LINE.
	for (size_t i = 0; i < num_lines; i++) {
		ctx->lines[2*(i+ctx->num_line)]   = line_indices[i*2]   + ctx->num_vertex;
		ctx->lines[2*(i+ctx->num_line)+1] = line_indices[i*2+1] + ctx->num_vertex;
	}
	
	// Insert TRI.
	for (size_t i = 0; i < num_tris; i++) {
		ctx->tris[3*(i+ctx->num_tri)]   = tri_indices[i*3]   + ctx->num_vertex;
		ctx->tris[3*(i+ctx->num_tri)+1] = tri_indices[i*3+1] + ctx->num_vertex;
		ctx->tris[3*(i+ctx->num_tri)+2] = tri_indices[i*3+2] + ctx->num_vertex;
	}
	
	ctx->num_vertex += num_vertices;
	ctx->num_line   += num_lines;
	ctx->num_tri    += num_tris;
}

// Adds a SHAPE to the DRAWING QUEUE.
void wf3d_mesh(wf3d_ctx_t *ctx, wf3d_shape_t *shape) {
	if (shape->num_tri)
		wf3d_tris(ctx, shape->num_vertex, shape->vertices, shape->num_tri, shape->tri_indices);
	else
		wf3d_lines(ctx, shape->num_vertex, shape->vertices, shape->num_lines, shape->line_indices);
}

// DRAWs everything in the DRAWING QUEUE.
void wf3d_render(pax_buf_t *to, pax_col_t color, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix) {
	// Get camera information.
	float focal = wf3d_get_foc(to, ctx);
	ctx->width  = to->width;
	ctx->height = to->height;
	ctx->mask   = ((color & 0xff0000) ? 0xff0000 : 0)
				| ((color & 0x00ff00) ? 0x00ff00 : 0)
				| ((color & 0x0000ff) ? 0x0000ff : 0);
	
	// Clear depth buffer.
	memset(ctx->depth, 255, sizeof(depth_t) * ctx->width * ctx->height);
	
	// Transform 3D points into 2D.
	vec3f_t *xform_vtx = malloc(sizeof(vec3f_t) * ctx->num_vertex);
	vec3f_t *proj_vtx  = malloc(sizeof(vec3f_t) * ctx->num_vertex);
	float max_depth = 0;
	for (size_t i = 0; i < ctx->num_vertex; i++) {
		vec3f_t raw_vtx = ctx->vertices[i];
		matrix_3d_transform(cam_matrix, &raw_vtx.x, &raw_vtx.y, &raw_vtx.z);
		xform_vtx[i] = raw_vtx;
		proj_vtx[i] = wf3d_xform(ctx, focal, raw_vtx);
		if (proj_vtx[i].z > max_depth) max_depth = proj_vtx[i].z;
	}
	
	// Set up PAX transform thingy.
	pax_push_2d(to);
	float scale = fminf(to->width, to->height);
	pax_apply_2d(to, matrix_2d_translate(to->width / 2.0, to->height / 2.0));
	pax_apply_2d(to, matrix_2d_scale(scale / 2, -scale / 2));
	
	const pax_shader_t wf3d_shader_depth = {
		.schema_version    =  1,
		.schema_complement = ~1,
		.renderer_id       = PAX_RENDERER_ID_SWR,
		.promise_callback  = NULL,
		.callback          = wf3d_shader_cb_depth,
		.callback_args     = ctx,
		.alpha_promise_0   = true,
		.alpha_promise_255 = true,
	};
	
	// Draw tris.
	matrix_3d_t ligt_mtx = matrix_3d_multiply(matrix_3d_rotate_x(-M_PI / 4), matrix_3d_rotate_y(-M_PI / 2));
	for (size_t i = 0; i < ctx->num_tri; i++) {
		size_t idx0 = ctx->tris[3*i];
		size_t idx1 = ctx->tris[3*i+1];
		size_t idx2 = ctx->tris[3*i+2];
		if (idx0 >= ctx->num_vertex || idx1 >= ctx->num_vertex || idx2 >= ctx->num_vertex) continue;
		
		// Compute normals.
		vec3f_t normals = wf3d_calc_tri_normals(xform_vtx[idx0], xform_vtx[idx1], xform_vtx[idx2]);
		
		if (normals.z <= 0 && proj_vtx[idx0].z >= 0 && proj_vtx[idx1].z >= 0 && proj_vtx[idx2].z >= 0) {
			// float avg_depth = (proj_vtx[idx0].z + proj_vtx[idx1].z + proj_vtx[idx2].z) / 3;
			// uint8_t part = 255 - 200 * (avg_depth / max_depth);
			uint8_t part = 255 - (matrix_3d_transform_inline(ligt_mtx, normals).z + 1) / 2 * 200;
			pax_tri_t depths = {
				.x0 = float_to_depth(proj_vtx[idx0].z, max_depth), .y0 = 0,
				.x1 = float_to_depth(proj_vtx[idx1].z, max_depth), .y1 = 0,
				.x2 = float_to_depth(proj_vtx[idx2].z, max_depth), .y2 = 0,
			};
			pax_shade_tri(
				to,
				// pax_col_rgb(127+normals.x*127, 16, 16),
				// pax_col_rgb(127+normals.x*32, 127+normals.y*127, 127+normals.z*127),
				pax_col_lerp(part, 0xff000000, color),
				&wf3d_shader_depth, &depths,
				proj_vtx[idx0].x, proj_vtx[idx0].y,
				proj_vtx[idx1].x, proj_vtx[idx1].y,
				proj_vtx[idx2].x, proj_vtx[idx2].y
			);
		}
	}
	
	// Draw lines.
	for (size_t i = 0; i < ctx->num_line; i++) {
		size_t start_idx = ctx->lines[2*i];
		size_t end_idx   = ctx->lines[2*i + 1];
		if (start_idx >= ctx->num_vertex || end_idx >= ctx->num_vertex) continue;
		
		if (proj_vtx[start_idx].z >= 0 && proj_vtx[end_idx].z >= 0) {
			float avg_depth = (proj_vtx[start_idx].z + proj_vtx[end_idx].z) / 2;
			uint8_t part = 255 - 100 * (avg_depth / max_depth);
			pax_shade_line(
				to, pax_col_lerp(part, 0xff000000, color),
				&wf3d_shader_maximum,
				proj_vtx[start_idx].x, proj_vtx[start_idx].y,
				proj_vtx[end_idx].x, proj_vtx[end_idx].y
			);
		}
	}
	
	// Clean up.
	pax_pop_2d(to);
	free(xform_vtx);
	free(proj_vtx);
}

// DRAWs everything in one color per eye.
void wf3d_render2(pax_buf_t *to, pax_col_t left_eye, pax_col_t right_eye, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix, float eye_dist) {
	wf3d_render(to, left_eye, ctx, matrix_3d_multiply(matrix_3d_translate(-eye_dist/2, 0, 0), cam_matrix));
	wf3d_render(to, right_eye, ctx, matrix_3d_multiply(matrix_3d_translate(eye_dist/2, 0, 0), cam_matrix));
}



// Calculates the normals for a 3D triangle.
vec3f_t wf3d_calc_tri_normals(vec3f_t p0, vec3f_t p1, vec3f_t p2) {
	vec3f_t a = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	vec3f_t b = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
	
	// Make some normals.
	vec3f_t normals = {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
	
	// Make the length 1.
	float mul = 1.0 / sqrtf(normals.x * normals.x + normals.y * normals.y + normals.z * normals.z);
	normals.x *= mul;
	normals.y *= mul;
	normals.z *= mul;
	return normals;
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
	size_t num_tri    = 2 * latitude_cuts * longitude_cuts;
	
	// Allocate memory.
	size_t vertices_size = num_vertex * sizeof(vec3f_t);
	size_t tris_size     = num_tri * sizeof(size_t) * 3;
	size_t lines_size    = num_line * sizeof(size_t) * 2;
	size_t memory        = (size_t) malloc(sizeof(wf3d_shape_t) + vertices_size + tris_size + lines_size);
	wf3d_shape_t *shape        = (void *)  memory;
	vec3f_t      *vertices     = (void *) (memory + sizeof(wf3d_shape_t));
	size_t       *tri_indices  = (void *) (memory + sizeof(wf3d_shape_t) + vertices_size);
	size_t       *line_indices = (void *) (memory + sizeof(wf3d_shape_t) + vertices_size + tris_size);
	
	// Generate vertices.
	vertices[0] = (vec3f_t) {0,  1, 0};
	vertices[1] = (vec3f_t) {0, -1, 0};
	
	vec3f_t vtx = {0, 1, 0};
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
	vertices[0] = matrix_3d_transform_inline(cur_mtx, vertices[0]);
	vertices[1] = matrix_3d_transform_inline(cur_mtx, vertices[1]);
	for (size_t i = 0; i < longitude_cuts; i++) {
		for (size_t x = 0; x < latitude_cuts; x++) {
			vertices[2 + x + i * latitude_cuts] = matrix_3d_transform_inline(cur_mtx, vertices[2 + x + i * latitude_cuts]);
		}
		
		cur_mtx = matrix_3d_multiply(cur_mtx, lon_rot_mtx);
	}
	
	// Generate line_indices.
	size_t line_lon_offset = 2 + latitude_cuts * 4;
	for (size_t lon = 0; lon < longitude_cuts; lon++) {
		size_t line_idx   = line_lon_offset * lon;
		size_t vertex_idx = 2 + lon * latitude_cuts;
		line_indices[line_idx]     = 0;
		line_indices[line_idx + 1] = vertex_idx;
		line_indices[line_idx + 2] = 1;
		line_indices[line_idx + 3] = vertex_idx + latitude_cuts - 1;
		line_indices[line_idx + 4] = vertex_idx;
		line_indices[line_idx + 5] = 2 + (lon * latitude_cuts + latitude_cuts) % (num_vertex - 2);
		line_idx += 6;
		for (size_t lat = 0; lat < latitude_cuts - 1; lat++) {
			line_indices[line_idx + lat * 4]     = lat + vertex_idx;
			line_indices[line_idx + lat * 4 + 1] = lat + vertex_idx + 1;
			line_indices[line_idx + lat * 4 + 2] = lat + vertex_idx + 1;
			line_indices[line_idx + lat * 4 + 3] = 2 + (lon * latitude_cuts + lat + latitude_cuts + 1) % (num_vertex - 2);
		}
	}
	
	// Generate tri_indices.
	for (size_t lon = 0; lon < longitude_cuts; lon++) {
		size_t tri_idx    = 6 * latitude_cuts * lon;
		size_t vertex_idx = 2 + lon * latitude_cuts;
		tri_indices[tri_idx]     = 0;
		tri_indices[tri_idx + 1] = 2 + (lon * latitude_cuts + latitude_cuts) % (num_vertex - 2);
		tri_indices[tri_idx + 2] = vertex_idx;
		tri_indices[tri_idx + 3] = 1;
		tri_indices[tri_idx + 4] = 2 + (lon * latitude_cuts + 2 * latitude_cuts - 1) % (num_vertex - 2);
		tri_indices[tri_idx + 5] = vertex_idx + latitude_cuts - 1;
		tri_idx += 6;
		for (size_t lat = 0; lat < latitude_cuts - 1; lat++) {
			tri_indices[lat*6 + tri_idx]     = lat + vertex_idx;
			tri_indices[lat*6 + tri_idx + 1] = 2 + (lon * latitude_cuts + lat + latitude_cuts + 1) % (num_vertex - 2);
			tri_indices[lat*6 + tri_idx + 2] = lat + vertex_idx + 1;
			tri_indices[lat*6 + tri_idx + 3] = lat + vertex_idx;
			tri_indices[lat*6 + tri_idx + 4] = 2 + (lon * latitude_cuts + lat + latitude_cuts) % (num_vertex - 2);
			tri_indices[lat*6 + tri_idx + 5] = 2 + (lon * latitude_cuts + lat + latitude_cuts + 1) % (num_vertex - 2);
		}
	}
	
	// Fill in the shape.
	shape->num_vertex   = num_vertex;
	shape->vertices     = vertices;
	shape->num_lines    = num_line;
	shape->line_indices = line_indices;
	shape->num_tri      = num_tri;
	shape->tri_indices  = tri_indices;
	return shape;
}
