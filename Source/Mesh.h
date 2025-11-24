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
	int64_t next;
	int64_t other_half;
	uint32_t face;
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

	int64_t num_edges;
	mp_edge_t *edges;
	int64_t *first_edge;	// Per vertex.

	uint8_t num_lod_levels;
	uint32_t num_faces[NM_MAX_LOD_LEVELS];
	mp_face_t *faces[NM_MAX_LOD_LEVELS];
} mp_mesh_t;

mp_mesh_t mp_mesh_initialise();
int mp_mesh_allocate(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
void mp_mesh_free(mp_mesh_t *mesh);
int mp_mesh_calculate_edges(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
void mp_mesh_free_edges(mp_mesh_t *mesh);
int mp_mesh_check_manifold(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);

#ifdef MP_DEBUG
void mp_mesh_print_short(FILE *file, mp_mesh_t *mesh);
void mp_mesh_print(FILE *file, mp_mesh_t *mesh);
#endif

#endif
