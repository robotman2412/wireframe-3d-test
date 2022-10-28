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

#include "obj.h"
#include <stdio.h>
#include <string.h>

static bool obj_nextline(FILE *fd, char *out_buf, size_t out_cap) {
	size_t out_written = 0;
	while (true) {
		int c = fgetc(fd);
		if (c == EOF || c == '\n') {
			goto fin;
		} else if (c == '\r') {
			c = fgetc(fd);
			if (c != '\n') fseek(fd, -1, SEEK_CUR);
		} else if (out_written < out_cap - 1) {
			out_buf[out_written] = c;
			out_written ++;
		}
	}
	
	fin:
	out_buf[out_written] = 0;
	return !feof(fd);
}

static size_t obj_count_prefix(FILE *fd, const char *prefix) {
	fseek(fd, 0, SEEK_SET);
	char tmp[strlen(prefix) + 1];
	size_t count = 0;
	
	while (obj_nextline(fd, tmp, sizeof(tmp))) {
		if (!strcmp(tmp, prefix)) count ++;
	}
	
	return count;
}

static bool obj_expect(FILE *fd, const char *prefix, char *buf, size_t cap) {
	while (obj_nextline(fd, buf, cap)) {
		if (!strncmp(prefix, buf, strlen(prefix))) return true;
	}
	return false;
}

static bool obj_lines_contains(size_t num_line, size_t *line_indices, size_t v0, size_t v1) {
	for (size_t i = 0; i < num_line; i++) {
		if ((line_indices[i*2] == v0 && line_indices[i*2+1] == v1) || (line_indices[i*2] == v1 && line_indices[i*2+1] == v0)) {
			return true;
		}
	}
	return false;
}

// Decodes an .obj model file and makes a wireframe version of it.
wf3d_shape_t *s3d_decode_obj(FILE *fd) {
	size_t num_vertex   = obj_count_prefix(fd, "v ");
	size_t num_triangle = obj_count_prefix(fd, "f ");
	
	// Allocate memory.
	size_t num_line      = num_triangle * 3;
	size_t vertices_size = num_vertex * sizeof(vec3f_t);
	size_t lines_size    = num_line * sizeof(size_t) * 2;
	size_t tris_size     = num_triangle * sizeof(size_t) * 3;
	size_t memory        = (size_t) malloc(sizeof(wf3d_shape_t) + vertices_size + lines_size + num_triangle);
	wf3d_shape_t *shape        = (void *)  memory;
	vec3f_t      *vertices     = (void *) (memory + sizeof(wf3d_shape_t));
	size_t       *tri_indices  = (void *) (memory + sizeof(wf3d_shape_t) + vertices_size);
	size_t       *line_indices = (void *) (memory + sizeof(wf3d_shape_t) + vertices_size + tris_size);
	
	char tmp[64];
	
	// Read in vertices.
	fseek(fd, 0, SEEK_SET);
	for (size_t i = 0; i < num_vertex; i++) {
		// Find a vertex def line.
		if (!obj_expect(fd, "v ", tmp, sizeof(tmp))) goto error;
		// Read in vertex data.
		vec3f_t vtx;
		int count = sscanf(tmp, "v %f %f %f", &vtx.x, &vtx.y, &vtx.z);
		if (count < 3) goto error;
		// Store to the vertex list.
		vertices[i] = vtx;
	}
	
	// Read in triangles.
	size_t real_line = 0;
	fseek(fd, 0, SEEK_SET);
	for (size_t i = 0; i < num_triangle; i++) {
		// Find a face def line.
		if (!obj_expect(fd, "f ", tmp, sizeof(tmp))) goto error;
		// Read in face data.
		int v0, v1, v2;
		int count = sscanf(tmp, "f %d %d %d", &v0, &v1, &v2);
		if (count < 3) count = sscanf(tmp, "f %d/%*d %d/%*d %d/%*d", &v0, &v1, &v2);
		if (count < 3) count = sscanf(tmp, "f %d//%*d %d//%*d %d//%*d", &v0, &v1, &v2);
		if (count < 3) count = sscanf(tmp, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &v0, &v1, &v2);
		if (count < 3) goto error;
		v0 --;
		v1 --;
		v2 --;
		
		// Add tri to the list.
		tri_indices[i*3]   = v0;
		tri_indices[i*3+1] = v1;
		tri_indices[i*3+2] = v2;
		
		// Add lines to the list.
		if (!obj_lines_contains(real_line, line_indices, v0, v1)) {
			line_indices[real_line*2]   = v0;
			line_indices[real_line*2+1] = v1;
			real_line ++;
		}
		if (!obj_lines_contains(real_line, line_indices, v2, v1)) {
			line_indices[real_line*2]   = v2;
			line_indices[real_line*2+1] = v1;
			real_line ++;
		}
		if (!obj_lines_contains(real_line, line_indices, v0, v2)) {
			line_indices[real_line*2]   = v0;
			line_indices[real_line*2+1] = v2;
			real_line ++;
		}
	}
	
	shape->num_vertex   = num_vertex;
	shape->vertices     = vertices;
	shape->num_lines    = real_line;
	shape->line_indices = line_indices;
	shape->num_tri      = num_triangle;
	shape->tri_indices  = tri_indices;
	return shape;
	
	error:
	free((void *) memory);
	return NULL;
}
