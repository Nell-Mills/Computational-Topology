#ifndef CT_MESH_H
#define CT_MESH_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>

typedef struct
{
	float x;
	float y;
	float z;
} ct_position_t;

typedef struct
{
	int8_t x;
	int8_t y;
	int8_t z;
} ct_normal_t;

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} ct_colour_t;

typedef struct
{
	float u;
	float v;
} ct_uv_t;

typedef struct
{
	uint32_t from;
	uint32_t to;
	uint32_t next;
	int64_t other_half;
} ct_edge_t;

typedef struct
{
	uint32_t p[3];
	uint32_t n[3];
	uint32_t c[3];
	uint32_t u[3];
} ct_face_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	char path[NM_MAX_PATH_LENGTH];

	uint8_t is_manifold;

	uint32_t num_vertices;
	ct_position_t *vertices;

	uint32_t num_normals;
	ct_normal_t *normals;

	uint32_t num_colours;
	ct_colour_t *colours;

	uint32_t num_uv_coordinates;
	ct_uv_t *uv_coordinates;

	uint32_t num_edges;
	ct_edge_t *edges;
	uint32_t *first_edge;	// Per vertex.

	uint8_t num_lod_levels;
	uint32_t num_faces[NM_MAX_LOD_LEVELS];
	ct_face_t *faces[NM_MAX_LOD_LEVELS];
} ct_mesh_t;

int ct_mesh_allocate(ct_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
void ct_mesh_free(ct_mesh_t *mesh);
int ct_mesh_check_validity(ct_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_mesh_calculate_edges(ct_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);

int ct_mesh_check_manifold(ct_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
uint32_t ct_mesh_triangle_fan_check(ct_mesh_t *mesh, uint32_t vertex, uint32_t vertex_degree);
uint32_t ct_mesh_get_edge_index(ct_edge_t *edge);
int64_t ct_mesh_get_next_vertex_edge(ct_mesh_t *mesh, uint32_t vertex, uint32_t edge);
int64_t ct_mesh_get_previous_vertex_edge(ct_mesh_t *mesh, uint32_t vertex, uint32_t edge);

int ct_mesh_edge_qsort_compare_from(const void *a, const void *b);
int ct_mesh_edge_qsort_compare_low_high(const void *a, const void *b);

#ifdef CT_DEBUG
void ct_mesh_print_short(FILE *file, ct_mesh_t *mesh);
void ct_mesh_print(FILE *file, ct_mesh_t *mesh);
#endif

#endif
