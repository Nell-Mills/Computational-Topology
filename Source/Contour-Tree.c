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

int ct_contour_tree_construct(ct_contour_tree_t *contour_tree, mp_mesh_t *mesh, int value_type,
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

	if (ct_contour_tree_join(&join_tree, mesh, value_type, vertex_values, error_message))
	{
		goto error;
	}
	if (ct_contour_tree_split(&split_tree, mesh, value_type, vertex_values, error_message))
	{
		goto error;
	}
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

int ct_contour_tree_join(ct_contour_tree_t *join_tree, mp_mesh_t *mesh, int value_type,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_contour_tree_split(ct_contour_tree_t *split_tree, mp_mesh_t *mesh, int value_type,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_contour_tree_merge(ct_contour_tree_t *contour_tree, ct_contour_tree_t *join_tree,
		ct_contour_tree_t *split_tree, char error_message[NM_MAX_ERROR_LENGTH])
{
	return 0;
}

int ct_qsort_compare_float(const void *a, const void *b)
{
	ct_vertex_value_t left = *(ct_vertex_value_t *)a;
	ct_vertex_value_t right = *(ct_vertex_value_t *)b;

	if (left.value.f != right.value.f) { return ((left.value.f > right.value.f) * 2) - 1; }
	return ((left.vertex > right.vertex) * 2) - 1;
}

int ct_qsort_compare_int(const void *a, const void *b)
{
	ct_vertex_value_t left = *(ct_vertex_value_t *)a;
	ct_vertex_value_t right = *(ct_vertex_value_t *)b;

	if (left.value.i != right.value.i) { return ((left.value.i > right.value.i) * 2) - 1; }
	return ((left.vertex > right.vertex) * 2) - 1;
}

int ct_qsort_compare_uint(const void *a, const void *b)
{
	ct_vertex_value_t left = *(ct_vertex_value_t *)a;
	ct_vertex_value_t right = *(ct_vertex_value_t *)b;

	if (left.value.u != right.value.u) { return ((left.value.u > right.value.u) * 2) - 1; }
	return ((left.vertex > right.vertex) * 2) - 1;
}

void ct_vertex_values_sort(int value_type, uint32_t num_values, ct_vertex_value_t *vertex_values)
{
	if (value_type == CT_VALUE_TYPE_FLOAT)
	{
		qsort(vertex_values, num_values, sizeof(vertex_values[0]), ct_qsort_compare_float);
	}
	else if (value_type == CT_VALUE_TYPE_INT)
	{
		qsort(vertex_values, num_values, sizeof(vertex_values[0]), ct_qsort_compare_int);
	}
	else
	{
		qsort(vertex_values, num_values, sizeof(vertex_values[0]), ct_qsort_compare_uint);
	}
}

#ifdef CT_DEBUG
void ct_contour_tree_print(FILE *file, ct_contour_tree_t *contour_tree)
{
	fprintf(file, "ct_contour_tree_print() - not implemented.\n");
}

void ct_vertex_value_print(FILE *file, int value_type, ct_vertex_value_t value)
{
	if (value_type == CT_VALUE_TYPE_FLOAT) { fprintf(file, "%f", value.value.f); }
	else if (value_type == CT_VALUE_TYPE_INT) { fprintf(file, "%d", value.value.i); }
	else { fprintf(file, "%u", value.value.u); }
}

void ct_vertex_values_print(FILE *file, int value_type, uint32_t num_values,
			ct_vertex_value_t *vertex_values, int indexed)
{
	if (num_values > 0)
	{
		fprintf(file, "Ordered vertices:\n");
		if (indexed) { fprintf(file, "%d: ", vertex_values[0].vertex); }
		ct_vertex_value_print(file, value_type, vertex_values[0]);
	}
	else
	{
		fprintf(file, "No ordered vertices to print.\n");
		return;
	}

	// If number of vertices exceeds 100, just print first 25 and last 25:
	if (num_values > 100)
	{
		for (uint32_t i = 1; i < 25; i++)
		{
			if (indexed) { fprintf(file, "\n%d: ", vertex_values[i].vertex); }
			else { fprintf(file, ", "); }
			ct_vertex_value_print(file, value_type, vertex_values[i]);
		}
		fprintf(file, "\n . . . \n");

		if (indexed) { fprintf(file, "%d: ", vertex_values[num_values - 25].vertex); }
		ct_vertex_value_print(file, value_type, vertex_values[num_values - 25]);
		for (uint32_t i = num_values - 24; i < num_values; i++)
		{
			if (indexed) { fprintf(file, "\n%d: ", vertex_values[i].vertex); }
			else { fprintf(file, ", "); }
			ct_vertex_value_print(file, value_type, vertex_values[i]);
		}
	}
	else
	{
		for (uint32_t i = 1; i < num_values; i++)
		{
			if (indexed) { fprintf(file, "\n%d: ", vertex_values[i].vertex); }
			else { fprintf(file, ", "); }
			ct_vertex_value_print(file, value_type, vertex_values[i]);
		}
	}
}
#endif
