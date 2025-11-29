#include "Contour-Tree.h"

int ct_contour_tree_allocate(ct_contour_tree_t *contour_tree,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	ct_contour_tree_free(contour_tree);

	contour_tree->nodes = malloc(contour_tree->num_nodes * sizeof(ct_contour_tree_node_t));
	contour_tree->arcs = malloc(contour_tree->num_nodes * sizeof(ct_contour_tree_arc_t));
	if (!contour_tree->nodes || !contour_tree->arcs)
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

	if (contour_tree->arcs)
	{
		free(contour_tree->arcs);
		contour_tree->arcs = NULL;
	}
}

int ct_contour_tree_construct(ct_contour_tree_t *contour_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	// Note that this expects an ordered set of vertex values.
	ct_contour_tree_t join_tree = {0};
	ct_contour_tree_t split_tree = {0};

	if (mesh->num_vertices < 3)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" has too few vertices to build a contour tree.", mesh->name);
		goto error;
	}

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
	return 0;

	// Cleanup, failure:
	error:
	ct_contour_tree_free(contour_tree);
	ct_contour_tree_free(&join_tree);
	ct_contour_tree_free(&split_tree);
	return -1;
}

int ct_contour_tree_join(ct_contour_tree_t *join_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	// Indexing into disjoint set via original vertex index, NOT sorted index.
	ct_disjoint_set_t disjoint_set = {0};
	disjoint_set.num_elements = mesh->num_vertices;
	if (ct_disjoint_set_allocate(&disjoint_set, error_message)) { return -1; }

	int64_t current_edge;
	uint32_t current_vertex;
	uint32_t current_component;
	uint32_t adjacent_vertex;
	uint32_t adjacent_component;
	uint32_t previous_adjacent_vertex;

	for (int64_t i = (mesh->num_vertices - 1); i >= 0; i--)
	{
		current_vertex = vertex_values[i].vertex;
		current_edge = mesh->first_edge[current_vertex];
		adjacent_vertex = current_vertex;
		while (1)
		{
			previous_adjacent_vertex = adjacent_vertex;
			if (mesh->edges[current_edge].from == current_vertex)
			{
				adjacent_vertex = mesh->edges[current_edge].to;
			}
			else { adjacent_vertex = mesh->edges[current_edge].from; }

			current_component = ct_disjoint_set_find(current_vertex, &disjoint_set);
			adjacent_component = ct_disjoint_set_find(adjacent_vertex, &disjoint_set);

			if ((vertex_values[adjacent_vertex].vertex_map > i) &&
				(current_component != adjacent_component) &&
				(adjacent_vertex != previous_adjacent_vertex))
			{
				ct_disjoint_set_union(current_component, adjacent_component,
									&disjoint_set);
				// TODO add edge to join tree.
				adjacent_component = ct_disjoint_set_find(adjacent_vertex,
									&disjoint_set);
				disjoint_set.lowest[adjacent_component] = current_vertex;
			}

			current_edge = mp_mesh_get_next_vertex_edge(mesh, current_vertex,
									current_edge);
			if (current_edge == -1) { break; }
			if (current_edge == mesh->first_edge[current_vertex]) { break; }
			if (mesh->edges[current_edge].other_half ==
				mesh->first_edge[current_vertex])
			{
				break;
			}
		}
	}

	#ifdef CT_DEBUG
	ct_disjoint_set_print(stdout, &disjoint_set);
	#endif

	ct_disjoint_set_free(&disjoint_set);
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

	// Map each vertex value to original vertex index:
	for (uint32_t i = 0; i < num_values; i++)
	{
		vertex_values[vertex_values[i].vertex].vertex_map = i;
	}
}

int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	disjoint_set->parent = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->rank = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->lowest = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	if (!disjoint_set->parent || !disjoint_set->rank || !disjoint_set->lowest)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for disjoint set.");
		return -1;
	}

	ct_disjoint_set_reset(disjoint_set);

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

	if (disjoint_set->lowest)
	{
		free(disjoint_set->lowest);
		disjoint_set->lowest = NULL;
	}
}

void ct_disjoint_set_reset(ct_disjoint_set_t *disjoint_set)
{
	for (uint32_t i = 0; i < disjoint_set->num_elements; i++)
	{
		disjoint_set->parent[i] = i;
		disjoint_set->rank[i] = 0;
		disjoint_set->lowest[i] = i;
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
		fprintf(file, "%u:\t%f\t(%u)", vertex_values[0].vertex, vertex_values[0].value,
								vertex_values[0].vertex_map);
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
			fprintf(file, "\n%u:\t%f\t(%u)", vertex_values[i].vertex,
				vertex_values[i].value, vertex_values[i].vertex_map);
		}
		fprintf(file, "\n. . . \n");

		fprintf(file, "\n%u:\t%f\t(%u)", vertex_values[num_values - 10].vertex,
					vertex_values[num_values - 10].value,
					vertex_values[num_values - 10].vertex_map);
		for (uint32_t i = num_values - 9; i < num_values; i++)
		{
			fprintf(file, "\n%u:\t%f\t(%u)", vertex_values[i].vertex,
				vertex_values[i].value, vertex_values[i].vertex_map);
		}
	}
	else
	{
		for (uint32_t i = 1; i < num_values; i++)
		{
			fprintf(file, "\n%u:\t%f\t(%u)", vertex_values[i].vertex,
				vertex_values[i].value, vertex_values[i].vertex_map);
		}
	}
}

void ct_disjoint_set_print(FILE *file, ct_disjoint_set_t *disjoint_set)
{
	fprintf(file, "Number of elements: %u\n", disjoint_set->num_elements);
	for (uint32_t i = 0; i < disjoint_set->num_elements; i++)
	{
		// Flatten structure and only print root elements:
		ct_disjoint_set_find(i, disjoint_set);
		if (disjoint_set->parent[i] != i) { continue; }
		fprintf(file, "Vertex: %u\t-->\tParent: %u,\tRank: %u,\tLowest: %u\n", i,
			disjoint_set->parent[i], disjoint_set->rank[i], disjoint_set->lowest[i]);
	}
}
#endif
