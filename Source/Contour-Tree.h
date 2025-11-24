#ifndef CT_CONTOUR_TREE_H
#define CT_CONTOUR_TREE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>
#include <Mesh-Processing/Mesh.h>

typedef struct
{
	float value;
	uint32_t vertex;
} ct_vertex_value_t;

typedef struct
{
	uint32_t vertex;
	uint8_t type;
} ct_contour_tree_node_t;

typedef struct
{
	uint32_t from;
	uint32_t to;
} ct_contour_tree_arc_t;

typedef struct
{
	uint32_t num_nodes;
	ct_contour_tree_node_t *nodes;
	ct_contour_tree_arc_t *arcs;
} ct_contour_tree_t;

typedef struct
{
	uint32_t num_elements;
	uint32_t *parent;
	uint32_t *rank;
} ct_disjoint_set_t;

ct_contour_tree_t ct_contour_tree_initialise();
int ct_contour_tree_allocate(ct_contour_tree_t *tree, char error_message[NM_MAX_ERROR_LENGTH]);
void ct_contour_tree_free(ct_contour_tree_t *tree);

int ct_contour_tree_construct(ct_contour_tree_t *contour_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_join(ct_contour_tree_t *join_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_split(ct_contour_tree_t *split_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_merge(ct_contour_tree_t *contour_tree, ct_contour_tree_t *join_tree,
		ct_contour_tree_t *split_tree, char error_message[NM_MAX_ERROR_LENGTH]);

int ct_qsort_compare(const void *a, const void *b);
void ct_vertex_values_sort(uint32_t num_values, ct_vertex_value_t *vertex_values);

ct_disjoint_set_t ct_disjoint_set_initialise();
int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set,
		char error_message[NM_MAX_ERROR_LENGTH]);
void ct_disjoint_set_free(ct_disjoint_set_t *disjoint_set);

void ct_disjoint_set_reset(ct_disjoint_set_t *disjoint_set);
void ct_disjoint_set_union(uint32_t v1, uint32_t v2, ct_disjoint_set_t *disjoint_set);
uint32_t ct_disjoint_set_find(uint32_t v, ct_disjoint_set_t *disjoint_set);

#ifdef CT_DEBUG
void ct_contour_tree_print(FILE *file, ct_contour_tree_t *contour_tree);
void ct_vertex_values_print(FILE *file, uint32_t num_values, ct_vertex_value_t *vertex_values);
#endif

#endif
