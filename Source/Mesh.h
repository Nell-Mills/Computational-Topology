#ifndef CT_MESH_H
#define CT_MESH_H

#include <math.h>
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
} ct_vertex_t;

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
	uint32_t other_half;
} ct_edge_t;

typedef struct
{
	uint32_t v;
	uint32_t n;
	uint32_t c;
	uint32_t u;
} ct_face_vertex_t;

typedef ct_face_vertex_t ct_face_t[3];
typedef uint32_t ct_face_gpu_ready_t[3];

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];
	char path[NM_MAX_PATH_LENGTH];

	uint8_t is_manifold;

	uint32_t num_vertices;
	ct_vertex_t *vertices;

	uint32_t num_normals;
	ct_normal_t *normals;

	uint32_t num_colours;
	ct_colour_t *colours;

	uint32_t num_uvs;
	ct_uv_t *uvs;

	uint32_t num_edges;
	ct_edge_t *edges;
	uint32_t *first_edge;	// Per vertex.

	uint32_t num_faces;
	ct_face_t *faces;
} ct_mesh_t;

typedef struct
{
	char name[NM_MAX_NAME_LENGTH];

	uint32_t num_vertices;
	ct_vertex_t *vertices;
	ct_normal_t *normals;
	ct_colour_t *colours;
	ct_uv_t *uvs;

	uint32_t num_faces;
	ct_face_gpu_ready_t *faces;
} ct_mesh_gpu_ready_t;

// Meshes:
int ct_mesh_allocate(ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
void ct_mesh_free(ct_mesh_t *mesh);
int ct_mesh_check_validity(ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_mesh_calculate_edges(ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_mesh_check_manifold(ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
uint32_t ct_mesh_triangle_fan_check(ct_mesh_t *mesh, uint32_t vertex, uint32_t vertex_degree);
uint32_t ct_mesh_get_edge_index(ct_edge_t *edge);
uint32_t ct_mesh_get_next_vertex_edge(ct_mesh_t *mesh, uint32_t vertex, uint32_t edge);
uint32_t ct_mesh_get_previous_vertex_edge(ct_mesh_t *mesh, uint32_t vertex, uint32_t edge);

// GPU-ready meshes:
int ct_mesh_gpu_ready_allocate(ct_mesh_gpu_ready_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
void ct_mesh_gpu_ready_free(ct_mesh_gpu_ready_t *mesh);
int ct_mesh_gpu_ready_check_validity(ct_mesh_gpu_ready_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_mesh_gpu_ready_vertex_normals(ct_mesh_gpu_ready_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_mesh_prepare_for_gpu(ct_mesh_t *mesh_old, ct_mesh_gpu_ready_t *mesh_new,
					char error[NM_MAX_ERROR_LENGTH]);

// Sorting:
int ct_mesh_edge_qsort_compare_from(const void *a, const void *b);
int ct_mesh_edge_qsort_compare_low_high(const void *a, const void *b);
int ct_mesh_face_vertex_qsort(const void *a, const void *b);

#ifdef CT_DEBUG
void ct_mesh_print_short(FILE *file, ct_mesh_t *mesh);
void ct_mesh_print(FILE *file, ct_mesh_t *mesh);
void ct_mesh_gpu_ready_print_short(FILE *file, ct_mesh_gpu_ready_t *mesh);
void ct_mesh_gpu_ready_print(FILE *file, ct_mesh_gpu_ready_t *mesh);
#endif

#endif
