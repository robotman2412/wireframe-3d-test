
#ifndef MATRIX3_H
#define MATRIX3_H

#ifdef __cplusplus
extern "C" {
#endif



// A matrix used for 3D transformations.
typedef union {
	struct {
		float a0, a1, a2, a3;
		float b0, b1, b2, b3;
		float c0, c1, c2, c3;
	};
	struct {
		float xx, yx, zx, dx;
		float xy, yy, zy, dy;
		float xz, yz, zz, dz;
	};
	float arr[12];
} matrix_3d_t;



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

// 3D matrix: applies the transformation that b represents on to a.
matrix_3d_t matrix_3d_multiply (matrix_3d_t a, matrix_3d_t b);
// 3D matrix: applies the transformation that a represents on to a point.
void        matrix_3d_transform(matrix_3d_t a, float *x, float *y, float *z);



#ifdef __cplusplus
}
#endif

#endif // MATRIX3_H
