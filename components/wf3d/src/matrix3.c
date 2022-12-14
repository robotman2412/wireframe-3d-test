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

#include "matrix3.h"
#include <math.h>

// 3D matrix: applies the transformation that b represents on to a.
matrix_3d_t matrix_3d_multiply(matrix_3d_t a, matrix_3d_t b) {
	return (matrix_3d_t) {
		.xx = a.xx * b.xx + a.yx * b.xy + a.zx * b.xz,
		.xy = a.xy * b.xx + a.yy * b.xy + a.zy * b.xz,
		.xz = a.xz * b.xx + a.yz * b.xy + a.zz * b.xz,
		
		.yx = a.xx * b.yx + a.yx * b.yy + a.zx * b.yz,
		.yy = a.xy * b.yx + a.yy * b.yy + a.zy * b.yz,
		.yz = a.xz * b.yx + a.yz * b.yy + a.zz * b.yz,
		
		.zx = a.xx * b.zx + a.yx * b.zy + a.zx * b.zz,
		.zy = a.xy * b.zx + a.yy * b.zy + a.zy * b.zz,
		.zz = a.xz * b.zx + a.yz * b.zy + a.zz * b.zz,
		
		.dx = a.xx * b.dx + a.yx * b.dy + a.zx * b.dz + a.dx,
		.dy = a.xy * b.dx + a.yy * b.dy + a.zy * b.dz + a.dy,
		.dz = a.xz * b.dx + a.yz * b.dy + a.zz * b.dz + a.dz,
	};
}

// 3D matrix: applies the transformation that a represents on to a point.
void matrix_3d_transform(matrix_3d_t a, float *x, float *y, float *z) {
	float x0 = *x, y0 = *y, z0 = *z;
	
	*x = x0 * a.xx + y0 * a.yx + z0 * a.zx + a.dx;
	*y = x0 * a.xy + y0 * a.yy + z0 * a.zy + a.dy;
	*z = x0 * a.xz + y0 * a.yz + z0 * a.zz + a.dz;
}

// 3D matrix: applies the transformation that a represents on to a point.
vec3f_t matrix_3d_transform_inline(matrix_3d_t a, vec3f_t vec) {
	float x0 = vec.x;
	float y0 = vec.y;
	float z0 = vec.z;
	return (vec3f_t) {
		x0 * a.xx + y0 * a.yx + z0 * a.zx + a.dx,
		x0 * a.xy + y0 * a.yy + z0 * a.zy + a.dy,
		x0 * a.xz + y0 * a.yz + z0 * a.zz + a.dz,
	};
}

// 3D rotation matrix: rotate around the X axis
matrix_3d_t matrix_3d_rotate_x(float angle) {
	float cos_res = cosf(angle);
	float sin_res = sinf(angle);
	return (matrix_3d_t) { .arr = {
		1, 0,        0,       0,
		0, cos_res, -sin_res, 0,
		0, sin_res,  cos_res, 0,
	}};
}

// 3D rotation matrix: rotate around the Y axis
matrix_3d_t matrix_3d_rotate_y(float angle) {
	float cos_res = cosf(angle);
	float sin_res = sinf(angle);
	return (matrix_3d_t) { .arr = {
		cos_res, 0, -sin_res, 0,
		0,       1,  0,       0,
		sin_res, 0,  cos_res, 0,
	}};
}

// 3D rotation matrix: rotate around the Z axis
matrix_3d_t matrix_3d_rotate_z(float angle) {
	float cos_res = cosf(angle);
	float sin_res = sinf(angle);
	return (matrix_3d_t) { .arr = {
		cos_res, -sin_res, 0, 0,
		sin_res,  cos_res, 0, 0,
		0,        0,       1, 0,
	}};
}

// 3D matrix: matrix inversion, such that inverted multiplied by input (in any order) is identity.
// Returns whether an inverse matrix was found.
bool matrix_3d_invert(matrix_3d_t *out_ptr, matrix_3d_t a) {
	matrix_3d_t out = matrix_3d_identity();
	
	//
	
	// Done!
	*out_ptr = out;
	return true;
}
