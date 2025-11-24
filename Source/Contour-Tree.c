#include "Contour-Tree.h"

ct_contour_tree_t ct_contour_tree_initialise()
{
	ct_contour_tree_t contour_tree;
	memset(&contour_tree, 0, sizeof(contour_tree));

	contour_tree.nodes = NULL;

	return contour_tree;
}

int ct_contour_tree_allocate(ct_contour_tree_t *contour_tree,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	ct_contour_tree_free(contour_tree);

	contour_tree->nodes = malloc(contour_tree->num_nodes * sizeof(ct_contour_tree_node_t));
	if (!contour_tree->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree.");
		return -1;
	}

	return 0;
}

void ct_contour_tree_free(ct_contour_tree_t *contour_tree)
{
	if (contour_tree->nodes)
	{
		free(contour_tree->nodes);
		contour_tree->nodes = NULL;
	}
}

int ct_contour_tree_construct(ct_contour_tree_t *contour_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	// Note that this expects an ordered set of vertex values.
	ct_contour_tree_t join_tree = ct_contour_tree_initialise();
	ct_contour_tree_t split_tree = ct_contour_tree_initialise();

	if (mesh->num_vertices < 3)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" has too few vertices to build a contour tree.", mesh->name);
		goto error;
	}

	int has_edges = 0;
	if (mesh->edges) { has_edges = 1; }
	if (!has_edges && mp_mesh_calculate_edges(mesh, error_message)) { goto error; }

	contour_tree->num_nodes = mesh->num_vertices;
	join_tree.num_nodes = mesh->num_vertices;
	split_tree.num_nodes = mesh->num_vertices;
	if (ct_contour_tree_allocate(contour_tree, error_message)) { goto error; }
	if (ct_contour_tree_allocate(&join_tree, error_message)) { goto error; }
	if (ct_contour_tree_allocate(&split_tree, error_message)) { goto error; }

	if (ct_contour_tree_join(&join_tree, mesh, vertex_values, error_message)) { goto error; }
	if (ct_contour_tree_split(&split_tree, mesh, vertex_values, error_message)) { goto error; }
	if (ct_contour_tree_merge(contour_tree, &join_tree, &split_tree, error_message))
	{
		goto error;
	}

	// Cleanup, success:
	ct_contour_tree_free(&join_tree);
	ct_contour_tree_free(&split_tree);
	if (!has_edges) { mp_mesh_free_edges(mesh); }
	return 0;

	// Cleanup, failure:
	error:
	ct_contour_tree_free(contour_tree);
	ct_contour_tree_free(&join_tree);
	ct_contour_tree_free(&split_tree);
	if (!has_edges) { mp_mesh_free_edges(mesh); }
	return -1;
}

int ct_contour_tree_join(ct_contour_tree_t *join_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_contour_tree_split(ct_contour_tree_t *split_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_contour_tree_merge(ct_contour_tree_t *contour_tree, ct_contour_tree_t *join_tree,
		ct_contour_tree_t *split_tree, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_qsort_compare(const void *a, const void *b)
{
	ct_vertex_value_t left = *(ct_vertex_value_t *)a;
	ct_vertex_value_t right = *(ct_vertex_value_t *)b;

	if (left.value != right.value) { return ((left.value > right.value) * 2) - 1; }
	return ((left.vertex > right.vertex) * 2) - 1;
}

void ct_vertex_values_sort(uint32_t num_values, ct_vertex_value_t *vertex_values)
{
	qsort(vertex_values, num_values, sizeof(vertex_values[0]), ct_qsort_compare);
}

ct_disjoint_set_t ct_disjoint_set_initialise()
{
	ct_disjoint_set_t disjoint_set;
	memset(&disjoint_set, 0, sizeof(disjoint_set));

	disjoint_set.parent	= NULL;
	disjoint_set.rank	= NULL;

	return disjoint_set;
}

int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	if (disjoint_set->num_elements < 1)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Tried to allocate disjoint set with no elements.");
		return -1;
	}

	disjoint_set->parent = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->rank = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	if (!disjoint_set->parent || !disjoint_set->rank)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for disjoint set.");
		return -1;
	}

	for (uint32_t i = 0; i < disjoint_set->num_elements; i++)
	{
		disjoint_set->parent[i] = i;
		disjoint_set->rank[i] = 0;
	}

	return 0;
}

void ct_disjoint_set_free(ct_disjoint_set_t *disjoint_set)
{
	if (disjoint_set->parent)
	{
		free(disjoint_set->parent);
		disjoint_set->parent = NULL;
	}

	if (disjoint_set->rank)
	{
		free(disjoint_set->rank);
		disjoint_set->rank = NULL;
	}
}

void ct_disjoint_set_union(uint32_t v1, uint32_t v2, ct_disjoint_set_t *disjoint_set)
{
	uint32_t v1_root = ct_disjoint_set_find(v1, disjoint_set);
	uint32_t v2_root = ct_disjoint_set_find(v2, disjoint_set);

	if (v1_root == v2_root) { return; }

	if (disjoint_set->rank[v1_root] < disjoint_set->rank[v2_root])
	{
		disjoint_set->parent[v1_root] = v2_root;
	}
	else if (disjoint_set->rank[v1_root] > disjoint_set->rank[v2_root])
	{
		disjoint_set->parent[v2_root] = v1_root;
	}
	else
	{
		disjoint_set->parent[v2_root] = v1_root;
		disjoint_set->rank[v1_root]++;
	}
}

uint32_t ct_disjoint_set_find(uint32_t v, ct_disjoint_set_t *disjoint_set)
{
	uint32_t root = disjoint_set->parent[v];
	if (disjoint_set->parent[root] != root)
	{
		return disjoint_set->parent[v] = ct_disjoint_set_find(root, disjoint_set);
	}
	return root;
}

#ifdef CT_DEBUG
void ct_contour_tree_print(FILE *file, ct_contour_tree_t *contour_tree)
{
	fprintf(file, "ct_contour_tree_print() - not implemented.\n");
}

void ct_vertex_values_print(FILE *file, uint32_t num_values, ct_vertex_value_t *vertex_values)
{
	if (num_values > 0)
	{
		fprintf(file, "Ordered vertices:\n");
		fprintf(file, "%d:\t%f", vertex_values[0].vertex, vertex_values[0].value);
	}
	else
	{
		fprintf(file, "No ordered vertices to print.\n");
		return;
	}

	// If number of vertices exceeds 20, just print first 10 and last 10:
	if (num_values > 20)
	{
		for (uint32_t i = 1; i < 10; i++)
		{
			fprintf(file, "\n%d:\t%f", vertex_values[i].vertex, vertex_values[i].value);
		}
		fprintf(file, "\n. . . \n");

		fprintf(file, "%d:\t%f", vertex_values[num_values - 10].vertex,
					vertex_values[num_values - 10].value);
		for (uint32_t i = num_values - 9; i < num_values; i++)
		{
			fprintf(file, "\n%d:\t%f", vertex_values[i].vertex, vertex_values[i].value);
		}
	}
	else
	{
		for (uint32_t i = 1; i < num_values; i++)
		{
			fprintf(file, "\n%d:\t%f", vertex_values[i].vertex, vertex_values[i].value);
		}
	}
}
#endif
