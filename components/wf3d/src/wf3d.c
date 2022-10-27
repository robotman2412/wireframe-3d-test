
#include "wf3d.h"
#include <math.h>
#include <string.h>
#include <esp_log.h>



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
	};
}

// DESTROYs a DRAWING QUEUE.
void wf3d_destroy(wf3d_ctx_t *ctx) {
	free(ctx->lines);
	free(ctx->vertices);
}

// CLEARs the DRAWING QUEUE.
void wf3d_clear(wf3d_ctx_t *ctx) {
	ctx->num_line     = 0;
	ctx->num_vertex   = 0;
	ctx->xfrom_vertex = 0;
}



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
	memcpy(&ctx->vertices[ctx->num_vertex], vertices, sizeof(vec3f_t) * num_vertices);
	ctx->num_vertex += num_vertices;
	
	// Insert LINE.
	for (size_t i = 0; i < num_lines; i++) {
		ctx->lines[2*(i+ctx->num_line)]   = indices[i*2]   + ctx->num_line;
		ctx->lines[2*(i+ctx->num_line)+1] = indices[i*2+1] + ctx->num_line;
	}
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
		
		// ESP_LOGI(
		// 	"wf3d", "Line from (%3.0f, %3.0f) to (%3.0f, %3.0f)",
		// 	xform_vtx[start_idx].x, xform_vtx[start_idx].y,
		// 	xform_vtx[end_idx].x, xform_vtx[end_idx].y
		// );
		
		if (xform_vtx[start_idx].z >= 0 && xform_vtx[end_idx].z >= 0) {
			pax_draw_line(
				to, color,
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
void wf3d_render2(pax_buf_t *to, pax_col_t left_eye, pax_col_t right_eye, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix) {
	wf3d_render(to, left_eye, ctx, matrix_3d_multiply(matrix_3d_translate(-0.1, 0, 0), cam_matrix));
	wf3d_render(to, right_eye, ctx, matrix_3d_multiply(matrix_3d_translate(0.1, 0, 0), cam_matrix));
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
