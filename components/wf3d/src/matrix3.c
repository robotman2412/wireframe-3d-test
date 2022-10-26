
#include "matrix3.h"

// 3D matrix: applies the transformation that b represents on to a.
matrix_3d_t matrix_3d_multiply(matrix_3d_t a, matrix_3d_t b) {
	
}

// 3D matrix: applies the transformation that a represents on to a point.
void matrix_3d_transform(matrix_3d_t a, float *x, float *y, float *z) {
	float x0 = *x, y0 = *y, z0 = *z;
	
	*x = x0 * a.xx + y0 * a.yx + z0 * a.zx + a.dx;
	*y = x0 * a.xy + y0 * a.yy + z0 * a.zy + a.dy;
	*z = x0 * a.xz + y0 * a.yz + z0 * a.zz + a.dz;
}
