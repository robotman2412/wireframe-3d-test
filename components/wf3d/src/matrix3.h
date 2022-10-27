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

#ifndef MATRIX3_H
#define MATRIX3_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>


// A matrix used for 3D transformations.
typedef union {
	struct {
		float xx, yx, zx, dx;
		float xy, yy, zy, dy;
		float xz, yz, zz, dz;
	};
	float arr[12];
} matrix_3d_t;


#include "wf3d.h"


// 3D identity matrix: Represents no change
static inline matrix_3d_t matrix_3d_identity() {
	return (matrix_3d_t) { .arr = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
	}};
}
// 3D scaling matrix: Scales around the current origin
static inline matrix_3d_t matrix_3d_scale(float x, float y, float z) {
	return (matrix_3d_t) { .arr = {
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, z, 0,
	}};
}
// 3D translation matrix: Moves around the origin
static inline matrix_3d_t matrix_3d_translate(float x, float y, float z) {
	return (matrix_3d_t) { .arr = {
		1, 0, 0, x,
		0, 1, 0, y,
		0, 0, 1, z,
	}};
}
// 3D rotation matrix: rotate around the X axis
matrix_3d_t matrix_3d_rotate_x(float angle);
// 3D rotation matrix: rotate around the Y axis
matrix_3d_t matrix_3d_rotate_y(float angle);
// 3D rotation matrix: rotate around the Z axis
matrix_3d_t matrix_3d_rotate_z(float angle);

// 3D matrix: applies the transformation that b represents on to a.
matrix_3d_t matrix_3d_multiply (matrix_3d_t a, matrix_3d_t b);
// 3D matrix: applies the transformation that a represents on to a point.
void        matrix_3d_transform(matrix_3d_t a, float *x, float *y, float *z);
// 3D matrix: applies the transformation that a represents on to a point.
vec3f_t     matrix_3d_transform_inline(matrix_3d_t a, vec3f_t vec);
// 3D matrix: matrix inversion, such that inverted multiplied by input (in any order) is identity.
// Returns whether an inverse matrix was found.
bool        matrix_3d_invert(matrix_3d_t *out_ptr, matrix_3d_t a);


#ifdef __cplusplus
}
#endif

#endif // MATRIX3_H
