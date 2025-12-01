#include "Contour-Tree.h"

/***********************
 * Contour/merge trees *
 ***********************/

int ct_tree_allocate(ct_tree_t *tree, char error_message[NM_MAX_ERROR_LENGTH])
{
	ct_tree_free(tree);

	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	tree->arcs = malloc(tree->num_nodes * sizeof(ct_tree_arc_t));
	if (!tree->nodes || !tree->arcs)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree.");
		return -1;
	}

	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));
	memset(tree->arcs, 0, tree->num_nodes * sizeof(ct_tree_arc_t));

	return 0;
}

void ct_tree_free(ct_tree_t *tree)
{
	if (tree->nodes)
	{
		free(tree->nodes);
		tree->nodes = NULL;
	}

	if (tree->arcs)
	{
		free(tree->arcs);
		tree->arcs = NULL;
		tree->num_arcs = 0;
	}
}

int ct_tree_node_is_critical(ct_tree_node_t *node)
{
	return (!((node->degree[0] == 1) && (node->degree[1] == 1)) &&
		!((node->degree[0] == 0) && (node->degree[1] == 0)));
}

int ct_contour_tree_construct(ct_tree_t *contour_tree, mp_mesh_t *mesh,
	ct_vertex_value_t *vertex_values, char error_message[NM_MAX_ERROR_LENGTH])
{
	// Note that this expects an ordered set of vertex values.
	ct_tree_t join_tree = {0};
	ct_tree_t split_tree = {0};
	ct_disjoint_set_t disjoint_set = {0};
	uint32_t *leaf_queue = NULL;

	if (mesh->num_vertices < 3)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" has too few vertices to build a contour tree.", mesh->name);
		goto error;
	}

	// Indexing into disjoint set via original vertex index, NOT sorted index.
	disjoint_set.num_elements = mesh->num_vertices;
	if (ct_disjoint_set_allocate(&disjoint_set, error_message)) { return -1; }

	contour_tree->num_nodes = mesh->num_vertices;
	join_tree.num_nodes = mesh->num_vertices;
	split_tree.num_nodes = mesh->num_vertices;
	if (ct_tree_allocate(contour_tree, error_message)) { goto error; }
	if (ct_tree_allocate(&join_tree, error_message)) { goto error; }
	if (ct_tree_allocate(&split_tree, error_message)) { goto error; }

	ct_merge_tree_construct(&join_tree, mesh, join_tree.num_nodes - 1, vertex_values,
									&disjoint_set);
	ct_disjoint_set_reset(&disjoint_set);
	ct_merge_tree_construct(&split_tree, mesh, 0, vertex_values, &disjoint_set);

	#ifdef CT_DEBUG
	fprintf(stdout, "\n\nJoin Tree:\n");
	ct_tree_print(stdout, &join_tree);
	fprintf(stdout, "\n\nSplit Tree:\n");
	ct_tree_print(stdout, &split_tree);
	#endif

	/* There is exactly one arc leading down/up from each node in the join/split tree except
	 * at minima/maxima. Augment the arc arrays with one extra arc leading from each of these
	 * nodes. Don't change degree. Now you can index into the arc array (sorted by node "from")
	 * via the node index. */

	if ((join_tree.num_arcs >= join_tree.num_nodes) ||
		(split_tree.num_arcs >= split_tree.num_nodes))
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Merge tree has too many arcs.");
		goto error;
	}

	uint32_t offset_join = 0;
	uint32_t offset_split = 0;
	for (uint32_t i = 0; i < join_tree.num_nodes; i++)
	{
		if (join_tree.nodes[i].degree[1] == 0)
		{
			join_tree.arcs[join_tree.num_arcs + offset_join].from = i;
			join_tree.arcs[join_tree.num_arcs + offset_join].to = i;
			offset_join++;
		}

		if (split_tree.nodes[i].degree[0] == 0)
		{
			split_tree.arcs[split_tree.num_arcs + offset_split].from = i;
			split_tree.arcs[split_tree.num_arcs + offset_split].to = i;
			offset_split++;
		}
	}

	qsort(join_tree.arcs, join_tree.num_arcs + offset_join, sizeof(join_tree.arcs[0]),
							ct_tree_arc_from_qsort_compare);
	qsort(split_tree.arcs, split_tree.num_arcs + offset_split, sizeof(split_tree.arcs[0]),
							ct_tree_arc_from_qsort_compare);

	// Create leaf queue (and fill in some contour tree node data):
	uint32_t num_leaves = 0;
	uint32_t first_leaf = 0;
	leaf_queue = malloc(join_tree.num_nodes * sizeof(uint32_t));
	if (!leaf_queue) { goto error; }
	for (uint32_t i = 0; i < join_tree.num_nodes; i++)
	{
		contour_tree->nodes[i].vertex = join_tree.nodes[i].vertex;
		if ((join_tree.nodes[i].degree[0] + split_tree.nodes[i].degree[1]) == 1)
		{
			leaf_queue[num_leaves] = i;
			num_leaves++;
		}
	}

	/* FIXME - sometimes nodes don't get added to the leaf queue - investigate. */

	// Merge join and split trees:
	uint32_t leaf;
	uint32_t node;
	while (num_leaves > 1)
	{
		leaf = leaf_queue[first_leaf];
		first_leaf++;
		num_leaves--;

		if (join_tree.nodes[leaf].degree[0] == 0) // Upper leaf.
		{
			node = join_tree.arcs[leaf].to;
			if (!join_tree.nodes[node].degree[0]) { continue; } // Prevent duplicates.
			join_tree.nodes[node].degree[0] -= 1;

			// Note - contour tree arcs point from lower to higher so swap from and to.
			contour_tree->arcs[contour_tree->num_arcs].from = node;
			contour_tree->arcs[contour_tree->num_arcs].to = leaf;
			contour_tree->num_arcs++;
			contour_tree->nodes[leaf].degree[1] += 1;
			contour_tree->nodes[node].degree[0] += 1;
		}
		else // Lower leaf.
		{
			node = split_tree.arcs[leaf].to;
			if (!split_tree.nodes[node].degree[1]) { continue; }
			split_tree.nodes[node].degree[1] -= 1;

			contour_tree->arcs[contour_tree->num_arcs].from = leaf;
			contour_tree->arcs[contour_tree->num_arcs].to = node;
			contour_tree->num_arcs++;
			contour_tree->nodes[leaf].degree[0] += 1;
			contour_tree->nodes[node].degree[1] += 1;
		}

		join_tree.nodes[leaf].degree[0] = 0;
		split_tree.nodes[leaf].degree[1] = 0;

		if ((join_tree.nodes[node].degree[0] + split_tree.nodes[node].degree[1]) == 1)
		{
			leaf_queue[first_leaf + num_leaves] = node;
			num_leaves++;
		}
	}

	// Sort arcs by "from" node:
	qsort(contour_tree->arcs, contour_tree->num_arcs, sizeof(contour_tree->arcs[0]),
							ct_tree_arc_from_qsort_compare);

	// Cleanup, success:
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_disjoint_set_free(&disjoint_set);
	free(leaf_queue);
	return 0;

	// Cleanup, failure:
	error:
	ct_tree_free(contour_tree);
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_disjoint_set_free(&disjoint_set);
	if (leaf_queue) { free(leaf_queue); }
	return -1;
}

void ct_merge_tree_construct(ct_tree_t *merge_tree, mp_mesh_t *mesh, uint32_t start_index,
			ct_vertex_value_t *vertex_values, ct_disjoint_set_t *disjoint_set)
{
	uint32_t i = start_index;
	uint8_t direction;
	uint32_t index_limit;
	int (*index_compare)(uint32_t left, uint32_t right);
	int (*index_increment)(uint32_t *index, uint32_t limit);

	if (i == 0)
	{
		// Sweep low to high:
		direction = 0;
		index_limit = merge_tree->num_nodes;
		index_compare = ct_index_compare_split;
		index_increment = ct_index_increment_split;
	}
	else
	{
		// Sweep high to low:
		direction = 1;
		index_limit = 0;
		index_compare = ct_index_compare_join;
		index_increment = ct_index_increment_join;
	}

	int64_t current_edge;
	uint32_t current_vertex;
	uint32_t current_component;
	uint32_t adjacent_vertex;
	uint32_t adjacent_component;
	uint32_t previous_adjacent_vertex;
	uint32_t num_unions;
	while (1)
	{
		current_vertex = vertex_values[i].vertex;
		current_edge = mesh->first_edge[current_vertex];
		adjacent_vertex = current_vertex;
		merge_tree->nodes[i].vertex = current_vertex;
		num_unions = 0;
		while (1)
		{
			previous_adjacent_vertex = adjacent_vertex;
			if (mesh->edges[current_edge].from == current_vertex)
			{
				adjacent_vertex = mesh->edges[current_edge].to;
			}
			else { adjacent_vertex = mesh->edges[current_edge].from; }

			current_component = ct_disjoint_set_find(current_vertex, disjoint_set);
			adjacent_component = ct_disjoint_set_find(adjacent_vertex, disjoint_set);

			if ((adjacent_vertex != previous_adjacent_vertex) &&
				index_compare(vertex_values[adjacent_vertex].vertex_map, i) &&
				(current_component != adjacent_component))
			{
				num_unions++;
				ct_disjoint_set_union(current_component, adjacent_component,
									disjoint_set);

				// Arcs go from high to low in join tree, low to high in split tree:
				merge_tree->arcs[merge_tree->num_arcs].from =
					vertex_values[disjoint_set->lowest[
					adjacent_component]].vertex_map;
				merge_tree->arcs[merge_tree->num_arcs].to = i;
				merge_tree->nodes[merge_tree->arcs[merge_tree->num_arcs].
							from].degree[direction] += 1;
				merge_tree->nodes[i].degree[!direction] += 1;
				merge_tree->num_arcs++;

				adjacent_component = ct_disjoint_set_find(adjacent_vertex,
									disjoint_set);
				disjoint_set->lowest[adjacent_component] = current_vertex;
			}

			current_edge = mp_mesh_get_next_vertex_edge(mesh, current_vertex,
									current_edge);
			if ((current_edge == -1) || (mesh->edges[current_edge].other_half ==
							mesh->first_edge[current_vertex]))
			{
				break;
			}
		}

		if (!index_increment(&i, index_limit)) { break; }
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n");
	ct_disjoint_set_print(stdout, disjoint_set);
	fprintf(stdout, "\n");
	#endif
}

int ct_index_compare_join(uint32_t left, uint32_t right)
{
	return (left > right);
}

int ct_index_compare_split(uint32_t left, uint32_t right)
{
	return (left < right);
}

int ct_index_increment_join(uint32_t *index, uint32_t limit)
{
	if (*index == limit) { return 0; }
	*index -= 1;
	return 1;
}

int ct_index_increment_split(uint32_t *index, uint32_t limit)
{
	*index += 1;
	if (*index == limit) { return 0; }
	return 1;
}

/***********
 * Sorting *
 ***********/

int ct_tree_arc_from_qsort_compare(const void *a, const void *b)
{
	ct_tree_arc_t left = *(ct_tree_arc_t *)a;
	ct_tree_arc_t right = *(ct_tree_arc_t *)b;

	if (left.from != right.from) { return ((left.from > right.from) * 2) - 1; }
	return 0;
}

int ct_tree_arc_to_qsort_compare(const void *a, const void *b)
{
	ct_tree_arc_t left = *(ct_tree_arc_t *)a;
	ct_tree_arc_t right = *(ct_tree_arc_t *)b;

	if (left.to != right.to) { return ((left.to > right.to) * 2) - 1; }
	return 0;
}

int ct_vertex_values_qsort_compare(const void *a, const void *b)
{
	ct_vertex_value_t left = *(ct_vertex_value_t *)a;
	ct_vertex_value_t right = *(ct_vertex_value_t *)b;

	if (left.value != right.value) { return ((left.value > right.value) * 2) - 1; }
	return ((left.vertex > right.vertex) * 2) - 1;
}

void ct_vertex_values_sort(uint32_t num_values, ct_vertex_value_t *vertex_values)
{
	qsort(vertex_values, num_values, sizeof(vertex_values[0]), ct_vertex_values_qsort_compare);

	// Map each vertex value to original vertex index:
	for (uint32_t i = 0; i < num_values; i++)
	{
		vertex_values[vertex_values[i].vertex].vertex_map = i;
	}
}

/*****************
 * Disjoint sets *
 *****************/

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
/*********
 * Debug *
 *********/

void ct_tree_print(FILE *file, ct_tree_t *tree)
{
	// Note - only printing critical points.
	fprintf(file, "%u Arcs:\n", tree->num_arcs);
	for (uint32_t i = 0; i < tree->num_arcs; i++)
	{
		if (!ct_tree_node_is_critical(&(tree->nodes[tree->arcs[i].from])) &&
			!ct_tree_node_is_critical(&(tree->nodes[tree->arcs[i].to])))
		{
			continue;
		}
		fprintf(file, "Arc %u: %u --> %u\n", i, tree->arcs[i].from, tree->arcs[i].to);
	}

	fprintf(file, "\n%u Nodes:\n", tree->num_nodes);
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		if (!ct_tree_node_is_critical(&(tree->nodes[i]))) { continue; }
		fprintf(file, "Node %u: %u -> Up = %u, Down = %u\n", i, tree->nodes[i].vertex,
					tree->nodes[i].degree[0], tree->nodes[i].degree[1]);
	}
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
