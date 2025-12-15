#ifndef CT_CONTOUR_TREE_H
#define CT_CONTOUR_TREE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>

#include "Mesh.h"

#define CT_NODE_TYPE_DELETED	-1
#define CT_NODE_TYPE_REGULAR	 0
#define CT_NODE_TYPE_MINIMUM	 1
#define CT_NODE_TYPE_MAXIMUM	 2
#define CT_NODE_TYPE_SADDLE	 3

typedef struct
{
	float value;
	uint32_t node_to_vertex;
	uint32_t vertex_to_node;
	uint32_t degree[2];	// Up[0], down[1].
	uint32_t first_arc[2];
} ct_tree_node_t;

typedef struct
{
	uint32_t num_nodes;
	ct_tree_node_t *nodes;	// Allocated in ct_tree_scalar_function_<X>.

	uint32_t num_arcs;
	uint32_t *arcs;		// Allocated during tree construction.

	uint32_t num_roots;	// One tree per disconnected component in the mesh.
	uint32_t *roots;	// Allocated during tree construction. Low in join, high in split.
} ct_tree_t;

typedef struct
{
	uint32_t num_elements;
	uint32_t *parent;
	uint32_t *rank;
	uint32_t *extremum;	// Lowest in join tree, highest in split tree.
} ct_disjoint_set_t;

// Tree management:
void ct_tree_free(ct_tree_t *tree);
int8_t ct_tree_get_node_type(ct_tree_node_t *node);
int ct_tree_copy_nodes(ct_tree_t *from, ct_tree_t *to, char error[NM_MAX_ERROR_LENGTH]);
int ct_tree_nodes_qsort_compare(const void *a, const void *b);
int ct_tree_reduce_to_critical(ct_tree_t *from, ct_tree_t *to);

// Tree construction:
int ct_merge_tree_construct(ct_tree_t *merge_tree, ct_mesh_t *mesh,
	uint32_t start_index, char error[NM_MAX_ERROR_LENGTH]);
int ct_contour_tree_construct(ct_tree_t *contour_tree, ct_tree_t *join_tree,
	ct_tree_t *split_tree, char error[NM_MAX_ERROR_LENGTH]);

int ct_index_compare_join(uint32_t left, uint32_t right);
int ct_index_compare_split(uint32_t left, uint32_t right);
int ct_index_increment_join(uint32_t *index, uint32_t limit);
int ct_index_increment_split(uint32_t *index, uint32_t limit);

// Scalar functions:
int ct_tree_scalar_function_x(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_tree_scalar_function_y(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
int ct_tree_scalar_function_z(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);

// Disjoint sets:
int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set, char error[NM_MAX_ERROR_LENGTH]);
void ct_disjoint_set_free(ct_disjoint_set_t *disjoint_set);
void ct_disjoint_set_union(uint32_t v1, uint32_t v2, ct_disjoint_set_t *disjoint_set);
uint32_t ct_disjoint_set_find(uint32_t v, ct_disjoint_set_t *disjoint_set);

#ifdef CT_DEBUG
void ct_tree_print_short(FILE *file, ct_tree_t *tree);
void ct_tree_print(FILE *file, ct_tree_t *tree);
int ct_tree_build_test_case(ct_tree_t *join_tree, ct_tree_t *split_tree,
				char error[NM_MAX_ERROR_LENGTH]);
void ct_tree_print_test_case(FILE *file, ct_tree_t *tree);
#endif

#endif
