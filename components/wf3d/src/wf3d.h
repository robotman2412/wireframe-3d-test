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

#ifndef WF3D_H
#define WF3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pax_gfx.h>

typedef uint16_t depth_t;

typedef struct {
	float x, y, z;
} vec3f_t;

// A 3D shape little collection.
typedef struct {
	// The amount of vertices in this shape.
	size_t   num_vertex;
	// The vertices of this shape.
	vec3f_t *vertices;
	// The amount of lines in this shape.
	size_t   num_lines;
	// The line_indices of line vertices, two per line.
	size_t  *line_indices;
	// The amount of triangles in this shape.
	size_t   num_tri;
	// The line_indices of triangle vertices, three per triangle.
	size_t  *tri_indices;
} wf3d_shape_t;

#include "matrix3.h"
#include "obj.h"



#define WF3D_INITIAL_VERTEX_CAP 64
#define WF3D_INITIAL_LINE_CAP   64
#define WF3D_INITIAL_TRI_CAP    64



typedef enum {
	// Preset focal depth.
	CAMERA_FOCAL_DEPTH,
	// Focal depth determined by horizontal field of view.
	CAMERA_HORIZONTAL_FOV,
	// Focal depth determined by vertical field of view.
	CAMERA_VERTICAL_FOV,
} cam_mode_t;

typedef struct matrix_stack_3d matrix_stack_3d_t;
// A simple linked list data structure used to store matrices in a stack.
struct matrix_stack_3d {
	matrix_stack_3d_t *parent;
	matrix_3d_t        value;
};

typedef struct {
	// The amount of vertices stored.
	size_t      num_vertex;
	// The amount of vertices that will fit.
	size_t      cap_vertex;
	// A list of all vertices.
	vec3f_t    *vertices;
	
	// The amount of lines stored.
	size_t      num_line;
	// The amount of lines that will fit.
	size_t      cap_line;
	// A list of all lines.
	size_t     *lines;
	
	// The amount of lines stored.
	size_t      num_tri;
	// The amount of lines that will fit.
	size_t      cap_tri;
	// A list of all lines.
	size_t     *tris;
	
	// The way in which to determine focal depth.
	cam_mode_t  cam_mode;
	// The variable which helps determine focal depth.
	float       cam_var;
	
	// MATRIX STACK used to SAVE MATRIX for later.
	matrix_stack_3d_t stack;
	// A DepthBuffer ;)
	depth_t    *depth;
	
	// Current WIDTH being rendered.
	int width;
	// Current HEIGHT being rendered.
	int height;
	// Current COLOR MASK being rendered.
	pax_col_t mask;
} wf3d_ctx_t;

typedef pax_vec1_t vec2f_t;



// MAKEs a new DRAWING QUEUE.
void wf3d_init    (wf3d_ctx_t *ctx);
// DESTROYs a DRAWING QUEUE.
void wf3d_destroy (wf3d_ctx_t *ctx);
// CLEARs the DRAWING QUEUE.
void wf3d_clear   (wf3d_ctx_t *ctx);

// Adds a line to the DRAWING QUEUE.
void wf3d_line    (wf3d_ctx_t *ctx, vec3f_t start, vec3f_t end);
// Adds multiple LINEs to the DRAWING QUEUE.
void wf3d_lines   (wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_lines, size_t *line_indices);
// Adds a triangle to the DRAWING QUEUE.
void wf3d_tri     (wf3d_ctx_t *ctx, vec3f_t a, vec3f_t b, vec3f_t c);
// Adds multiple TRIANGLES to the DRAWING QUEUE.
void wf3d_tris    (wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_tris, size_t *tri_indices);
// Adds multiple LINES and TRIANGLES to the DRAWING QUEUE.
void wf3d_add     (wf3d_ctx_t *ctx, size_t num_vertices, vec3f_t *vertices, size_t num_lines, size_t *line_indices, size_t num_tris, size_t *tri_indices);
// Adds a SHAPE to the DRAWING QUEUE.
void wf3d_mesh    (wf3d_ctx_t *ctx, wf3d_shape_t *shape);
// DRAWs everything in the DRAWING QUEUE.
void wf3d_render  (pax_buf_t *to, pax_col_t color, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix);
// DRAWs everything in one color per eye.
void wf3d_render2 (pax_buf_t *to, pax_col_t left_eye, pax_col_t right_eye, wf3d_ctx_t *ctx, matrix_3d_t cam_matrix, float eye_dist);

// Calculates the normals for a 3D triangle.
vec3f_t wf3d_calc_tri_normals(vec3f_t a, vec3f_t b, vec3f_t c);
// Determines the focal depth to use in the given context.
float wf3d_get_foc(pax_buf_t *buf, wf3d_ctx_t *ctx);
// Transforms a 3D point into 2D.
vec3f_t wf3d_xform(wf3d_ctx_t *ctx, float focal_depth, vec3f_t point);

// APPLY some MATRIX.
void wf3d_apply_3d(wf3d_ctx_t *ctx, matrix_3d_t mtx);
// PUSH to the MATRIX STACK to SAVE FOR LATER.
void wf3d_push_3d (wf3d_ctx_t *ctx);
// POP from the MATRIX STACK to RESTORE.
void wf3d_pop_3d  (wf3d_ctx_t *ctx);
// RESET the MATRIX STACK.
void wf3d_reset_3d(wf3d_ctx_t *ctx);

// Creates a UV sphere mesh.
wf3d_shape_t *s3d_uv_sphere(vec3f_t position, float radius, int latitude_cuts, int longitude_cuts);


#ifdef __cplusplus
}
#endif

#endif // WF3D_H
