#ifndef MP_MESH_H
#define MP_MESH_H

#include <NM-Config/Config.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	float x;
	float y;
	float z;
} mp_position_t;

typedef struct
{
	int8_t x;
	int8_t y;
	int8_t z;
} mp_normal_t;

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} mp_colour_t;

typedef struct
{
	float u;
	float v;
} mp_uv_t;

typedef struct
{
	uint32_t from;
	uint32_t to;
	uint32_t next;
	int64_t other_half;
} mp_edge_t;

typedef struct
{
	uint32_t p[3];
	uint32_t n[3];
	uint32_t c[3];
	uint32_t u[3];
} mp_face_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	char path[NM_MAX_PATH_LENGTH];

	uint8_t is_manifold;

	uint32_t num_vertices;
	mp_position_t *vertices;

	uint32_t num_normals;
	mp_normal_t *normals;

	uint32_t num_colours;
	mp_colour_t *colours;

	uint32_t num_uv_coordinates;
	mp_uv_t *uv_coordinates;

	uint32_t num_edges;
	mp_edge_t *edges;
	uint32_t *first_edge;	// Per vertex.

	uint8_t num_lod_levels;
	uint32_t num_faces[NM_MAX_LOD_LEVELS];
	mp_face_t *faces[NM_MAX_LOD_LEVELS];
} mp_mesh_t;

int mp_mesh_allocate(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
void mp_mesh_free(mp_mesh_t *mesh);
int mp_mesh_check_validity(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
int mp_mesh_calculate_edges(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);

int mp_mesh_check_manifold(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
uint32_t mp_mesh_triangle_fan_check(mp_mesh_t *mesh, uint32_t vertex, uint32_t vertex_degree);
uint32_t mp_mesh_get_edge_index(mp_edge_t *edge);
int64_t mp_mesh_get_next_vertex_edge(mp_mesh_t *mesh, uint32_t vertex, uint32_t edge);
int64_t mp_mesh_get_previous_vertex_edge(mp_mesh_t *mesh, uint32_t vertex, uint32_t edge);

int mp_mesh_edge_qsort_compare_from(const void *a, const void *b);
int mp_mesh_edge_qsort_compare_low_high(const void *a, const void *b);

#ifdef MP_DEBUG
void mp_mesh_print_short(FILE *file, mp_mesh_t *mesh);
void mp_mesh_print(FILE *file, mp_mesh_t *mesh);
#endif

#endif
