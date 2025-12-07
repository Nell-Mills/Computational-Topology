#ifndef CT_CONTOUR_TREE_H
#define CT_CONTOUR_TREE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <NM-Config/Config.h>

#include "Mesh.h"

typedef struct
{
	float value;
	uint32_t vertex;
	uint32_t vertex_map;	// Get sorted index from unsorted index (vertex).
} ct_vertex_value_t;

typedef struct
{
	uint32_t vertex;
	uint32_t degree[2];	// Up[0], down[1].
} ct_tree_node_t;

typedef struct
{
	uint32_t from;	// Node index.
	uint32_t to;
} ct_tree_arc_t;

typedef struct
{
	uint32_t num_nodes;
	uint32_t num_arcs;
	ct_tree_node_t *nodes;
	ct_tree_arc_t *arcs;
} ct_tree_t;

typedef struct
{
	uint32_t num_elements;
	uint32_t *parent;
	uint32_t *rank;
	uint32_t *extremum; // Lowest in join tree, highest in split tree.
} ct_disjoint_set_t;

// Contour/merge trees:
int ct_tree_allocate(ct_tree_t *tree, char error_message[NM_MAX_ERROR_LENGTH]);
void ct_tree_free(ct_tree_t *tree);
int ct_tree_node_is_critical(ct_tree_node_t *node);

int ct_contour_tree_construct(ct_tree_t *contour_tree, ct_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH]);
void ct_merge_tree_construct(ct_tree_t *merge_tree, ct_mesh_t *mesh, uint32_t start_index,
		ct_vertex_value_t *vertex_values, ct_disjoint_set_t *disjoint_set);

int ct_index_compare_join(uint32_t left, uint32_t right);
int ct_index_compare_split(uint32_t left, uint32_t right);
int ct_index_increment_join(uint32_t *index, uint32_t limit);
int ct_index_increment_split(uint32_t *index, uint32_t limit);

// TODO
typedef struct
{
	float value;
	uint32_t node_to_vertex;
	uint32_t vertex_to_node;
	uint32_t degree[2]; // Up[0], down[1].
	uint32_t first_arc; // Arcs array references connected nodes; up, then down.
} ct_tree_node_t_NEW;

typedef struct
{
	uint32_t num_nodes;
	ct_tree_node_t_NEW *nodes; // Allocated in ct_tree_scalar_function_<X>.

	uint32_t num_arcs;
	uint32_t *arcs; // Allocated during tree construction.

	uint32_t num_roots; // One tree per disconnected component in the mesh.
	uint32_t *roots; // Lowest-valued node in each tree. Allocated during tree construction.
} ct_tree_t_NEW;

// Tree management:
void ct_tree_free_NEW(ct_tree_t_NEW *tree);
void ct_tree_sort_nodes_NEW(ct_tree_t_NEW *tree);
int ct_tree_nodes_qsort_compare_NEW(const void *a, const void *b);
int ct_tree_node_is_critical_NEW(ct_tree_node_t_NEW *node);

// Tree construction:
int ct_merge_tree_construct_NEW(ct_tree_t_NEW *merge_tree, ct_mesh_t *mesh,
	uint32_t start_index, char error_message[NM_MAX_ERROR_LENGTH]);

// Vertex scalar functions:
int ct_tree_scalar_function_y_NEW(ct_tree_t_NEW *tree, ct_mesh_t *mesh,
			char error_message[NM_MAX_ERROR_LENGTH]);
int ct_tree_scalar_function_z_NEW(ct_tree_t_NEW *tree, ct_mesh_t *mesh,
			char error_message[NM_MAX_ERROR_LENGTH]);
// TODO

// Sorting:
int ct_tree_arc_from_qsort_compare(const void *a, const void *b);
int ct_tree_arc_to_qsort_compare(const void *a, const void *b);
int ct_vertex_values_qsort_compare(const void *a, const void *b);
void ct_vertex_values_sort(uint32_t num_values, ct_vertex_value_t *vertex_values);

// Disjoint sets:
int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set,
		char error_message[NM_MAX_ERROR_LENGTH]);
void ct_disjoint_set_free(ct_disjoint_set_t *disjoint_set);
void ct_disjoint_set_reset(ct_disjoint_set_t *disjoint_set);
void ct_disjoint_set_union(uint32_t v1, uint32_t v2, ct_disjoint_set_t *disjoint_set);
uint32_t ct_disjoint_set_find(uint32_t v, ct_disjoint_set_t *disjoint_set);

#ifdef CT_DEBUG
void ct_merge_trees_build_test_case(ct_tree_t *join_tree, ct_tree_t *split_tree);
void ct_merge_trees_print_test_case(FILE *file, ct_tree_t *tree);
void ct_tree_print(FILE *file, ct_tree_t *tree);
void ct_tree_print_NEW(FILE *file, ct_tree_t_NEW *tree);
void ct_vertex_values_print(FILE *file, uint32_t num_values, ct_vertex_value_t *vertex_values);
void ct_disjoint_set_print(FILE *file, ct_disjoint_set_t *disjoint_set);
#endif

#endif
