#ifndef CT_CONTOUR_TREE_H
#define CT_CONTOUR_TREE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>
#include <Mesh-Processing/Mesh.h>

#define CT_VALUE_TYPE_FLOAT	0
#define CT_VALUE_TYPE_INT	1
#define CT_VALUE_TYPE_UINT	2

typedef struct
{
	union
	{
		float f;
		int32_t i;
		uint32_t u;
	} value;
	uint32_t vertex;
} ct_vertex_value_t;

typedef struct
{
	uint32_t self;
	uint32_t parent;
	uint8_t type;
} ct_contour_tree_node_t;

typedef struct
{
	uint32_t num_nodes;
	ct_contour_tree_node_t *nodes;
} ct_contour_tree_t;

ct_contour_tree_t ct_contour_tree_initialise();
int ct_contour_tree_allocate(ct_contour_tree_t *tree, char error_message[NM_MAX_ERROR_LENGTH]);
void ct_contour_tree_free(ct_contour_tree_t *tree);

int ct_contour_tree_construct(ct_contour_tree_t *contour_tree, mp_mesh_t *mesh, int value_type,
		ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_join(ct_contour_tree_t *join_tree, mp_mesh_t *mesh, int value_type,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_split(ct_contour_tree_t *split_tree, mp_mesh_t *mesh, int value_type,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_merge(ct_contour_tree_t *contour_tree, ct_contour_tree_t *join_tree,
		ct_contour_tree_t *split_tree, char error_message[NM_MAX_ERROR_LENGTH]);

int ct_qsort_compare_float(const void *a, const void *b);
int ct_qsort_compare_int(const void *a, const void *b);
int ct_qsort_compare_uint(const void *a, const void *b);
void ct_vertex_values_sort(int value_type, uint32_t num_values, ct_vertex_value_t *vertex_values);

#ifdef CT_DEBUG
void ct_contour_tree_print(FILE *file, ct_contour_tree_t *contour_tree);
void ct_vertex_value_print(FILE *file, int value_type, ct_vertex_value_t value);
void ct_vertex_values_print(FILE *file, int value_type, uint32_t num_values,
			ct_vertex_value_t *vertex_values, int indexed);
#endif

#endif
