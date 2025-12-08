#include "Contour-Tree.h"

/*******************
 * Tree management *
 *******************/

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
	}

	if (tree->roots)
	{
		free(tree->roots);
		tree->roots = NULL;
	}
}

int ct_tree_node_is_critical(ct_tree_node_t *node)
{
	return (!((node->degree[0] == 1) && (node->degree[1] == 1)) &&
		!((node->degree[0] == 0) && (node->degree[1] == 0)));
}

int ct_tree_copy_nodes(ct_tree_t *from, ct_tree_t *to,
			char error_message[NM_MAX_ERROR_LENGTH])
{
	ct_tree_free(to);
	to->num_nodes = from->num_nodes;
	to->nodes = malloc(to->num_nodes * sizeof(ct_tree_node_t));
	if (!to->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for tree nodes.");
		return -1;
	}
	memcpy(to->nodes, from->nodes, to->num_nodes * sizeof(ct_tree_node_t));

	return 0;
}

void ct_tree_sort_nodes(ct_tree_t *tree)
{
	// Sort ascending by scalar value:
	qsort(tree->nodes, tree->num_nodes, sizeof(tree->nodes[0]), ct_tree_nodes_qsort_compare);

	// Construct 2-way mapping of node index --> vertex index:
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[tree->nodes[i].node_to_vertex].vertex_to_node = i;
	}
}

int ct_tree_nodes_qsort_compare(const void *a, const void *b)
{
	ct_tree_node_t left = *(ct_tree_node_t *)a;
	ct_tree_node_t right = *(ct_tree_node_t *)b;

	if (left.value != right.value) { return ((left.value > right.value) * 2) - 1; }
	return ((left.node_to_vertex > right.node_to_vertex) * 2) - 1;
}

/*********************
 * Tree construction *
 *********************/

int ct_merge_tree_construct(ct_tree_t *merge_tree, ct_mesh_t *mesh,
	uint32_t start_index, char error_message[NM_MAX_ERROR_LENGTH])
{
	if (!merge_tree->num_nodes || !merge_tree->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Merge tree has no nodes.");
		return -1;
	}

	ct_disjoint_set_t disjoint_set = {0};
	disjoint_set.num_elements = merge_tree->num_nodes;
	if (ct_disjoint_set_allocate(&disjoint_set, error_message)) { return -1; }

	merge_tree->arcs = malloc(merge_tree->num_nodes * 2 * sizeof(uint32_t));
	if (!merge_tree->arcs)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for merge tree arcs.");
		ct_disjoint_set_free(&disjoint_set);
		return -1;
	}
	memset(merge_tree->arcs, 0, merge_tree->num_nodes * 2 * sizeof(uint32_t));

	uint32_t i = start_index;
	uint32_t index_limit;
	uint8_t direction;
	int (*index_compare)(uint32_t left, uint32_t right);
	int (*index_increment)(uint32_t *index, uint32_t limit);

	if (i == 0)
	{
		// Sweep low to high (split tree):
		direction = 0;
		index_limit = merge_tree->num_nodes;
		index_compare = ct_index_compare_split;
		index_increment = ct_index_increment_split;
	}
	else
	{
		// Sweep high to low (join tree):
		direction = 1;
		index_limit = 0;
		index_compare = ct_index_compare_join;
		index_increment = ct_index_increment_join;
	}

	uint32_t current_arc = 0;
	uint32_t current_edge;
	uint32_t current_vertex;
	uint32_t current_component;
	uint32_t adjacent_vertex;
	uint32_t adjacent_node;
	uint32_t adjacent_component;
	uint32_t previous_adjacent_vertex;
	while (1)
	{
		merge_tree->nodes[i].first_arc = current_arc;
		if (direction == 0) { current_arc++; } // Allow for the up arc in the split tree.
		current_vertex = merge_tree->nodes[i].node_to_vertex;
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

			adjacent_node = merge_tree->nodes[adjacent_vertex].vertex_to_node;
			current_component = ct_disjoint_set_find(i, &disjoint_set);
			adjacent_component = ct_disjoint_set_find(adjacent_node, &disjoint_set);
			if ((adjacent_vertex != previous_adjacent_vertex) &&
				index_compare(adjacent_node, i) &&
				(current_component != adjacent_component))
			{
				ct_disjoint_set_union(current_component, adjacent_component,
									&disjoint_set);

				adjacent_node = disjoint_set.extremum[adjacent_component];
				merge_tree->arcs[current_arc] = adjacent_node;
				if (direction == 1)
				{
					// Arcs from high to low in join tree:
					merge_tree->arcs[merge_tree->nodes[adjacent_node].
						first_arc + merge_tree->nodes[adjacent_node].
						degree[0]] = i;
				}
				else
				{
					// Arcs from low to high in split tree:
					merge_tree->arcs[merge_tree->nodes[adjacent_node].
						first_arc] = i;
				}
				current_arc++;
				merge_tree->num_arcs++;
				merge_tree->nodes[i].degree[!direction]++;
				merge_tree->nodes[adjacent_node].degree[direction]++;

				adjacent_component = ct_disjoint_set_find(adjacent_node,
									&disjoint_set);
				disjoint_set.extremum[adjacent_component] = i;
			}

			current_edge = ct_mesh_get_next_vertex_edge(mesh, current_vertex,
									current_edge);
			if ((current_edge == UINT32_MAX) || (mesh->edges[current_edge].other_half ==
								mesh->first_edge[current_vertex]))
			{
				break;
			}
		}
		if (direction == 1) { current_arc++; } // Allow for the down arc in the join tree.

		if (!index_increment(&i, index_limit)) { break; }
	}

	// Repair up arcs in the split tree. If the up degree is 0, an element was still added:
	if (direction == 0)
	{
		for (uint32_t i = 0; i < merge_tree->num_nodes; i++)
		{
			if (!merge_tree->nodes[i].degree[0])
			{
				// By definition, there must be exactly 1 down arc - swap them:
				merge_tree->arcs[merge_tree->nodes[i].first_arc] =
					merge_tree->arcs[merge_tree->nodes[i].first_arc + 1];
			}
		}
	}

	// Fill in root information. Allocate more memory than needed for now:
	merge_tree->roots = malloc(merge_tree->num_nodes * sizeof(uint32_t));
	if (!merge_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for tree roots.");
		ct_disjoint_set_free(&disjoint_set);
		return -1;
	}

	for (uint32_t j = 0; j < disjoint_set.num_elements; j++)
	{
		if (disjoint_set.parent[j] == j)
		{
			merge_tree->roots[merge_tree->num_roots] = disjoint_set.extremum[j];
			merge_tree->num_roots++;
		}
	}

	merge_tree->roots = realloc(merge_tree->roots, merge_tree->num_roots * sizeof(uint32_t));
	if (!merge_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not reallocate memory for tree roots.");
		ct_disjoint_set_free(&disjoint_set);
		return -1;
	}

	ct_disjoint_set_free(&disjoint_set);
	return 0;
}

int ct_contour_tree_construct(ct_tree_t *contour_tree, ct_tree_t *join_tree,
	ct_tree_t *split_tree, char error_message[NM_MAX_ERROR_LENGTH])
{
	// FIXME - not enough arcs in some contour trees.

	/*****************
	 * Sanity checks *
	 *****************/

	if (!join_tree->nodes || !join_tree->arcs || !join_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Join tree is incomplete.");
		return -1;
	}
	if (!split_tree->nodes || !split_tree->arcs || !split_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Split tree is incomplete.");
		return -1;
	}
	if (join_tree->num_arcs != (join_tree->num_nodes - join_tree->num_roots))
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Join tree has incorrect number of arcs.");
		return -1;
	}
	if (split_tree->num_arcs != (split_tree->num_nodes - split_tree->num_roots))
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Split tree has incorrect number of arcs.");
		return -1;
	}
	if ((join_tree->num_nodes != split_tree->num_nodes) ||
		(join_tree->num_arcs != split_tree->num_arcs) ||
		(join_tree->num_roots != split_tree->num_roots))
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH, "Join and split tree do not match.");
		return -1;
	}

	/***************************
	 * Contour tree node setup *
	 ***************************/

	contour_tree->num_nodes = join_tree->num_nodes;
	contour_tree->nodes = malloc(contour_tree->num_nodes * sizeof(ct_tree_node_t));
	if (!contour_tree->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree nodes.");
		return -1;
	}
	memset(contour_tree->nodes, 0, contour_tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		contour_tree->nodes[i].value = join_tree->nodes[i].value;
		contour_tree->nodes[i].node_to_vertex = join_tree->nodes[i].node_to_vertex;
		contour_tree->nodes[i].vertex_to_node = join_tree->nodes[i].vertex_to_node;

		// Degree information will be accumulated during merging.

		if (i > 0)
		{
			contour_tree->nodes[i].first_arc = contour_tree->nodes[i - 1].first_arc +
							join_tree->nodes[i - 1].degree[0] +
							split_tree->nodes[i - 1].degree[1];
		}
	}

	/***********************************
	 * Contour tree arc and root setup *
	 ***********************************/

	contour_tree->num_arcs = join_tree->num_arcs;
	contour_tree->arcs = malloc(contour_tree->num_arcs * 2 * sizeof(uint32_t));
	if (!contour_tree->arcs)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree arcs.");
		return -1;
	}
	memset(contour_tree->arcs, 0, contour_tree->num_arcs * 2 * sizeof(uint32_t));
	contour_tree->num_arcs = 0;

	contour_tree->num_roots = join_tree->num_roots;
	contour_tree->roots = malloc(contour_tree->num_roots * sizeof(uint32_t));
	if (!contour_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree roots.");
		return -1;
	}
	memcpy(contour_tree->roots, join_tree->roots, contour_tree->num_roots * sizeof(uint32_t));

	/*****************************
	 * Contour tree construction *
	 *****************************/

	// Track original up degrees of nodes (needed for down degree indexing):
	uint32_t *up_degrees = malloc(contour_tree->num_nodes * sizeof(uint32_t));
	if (!up_degrees)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for up degrees.");
		return -1;
	}

	// Create leaf queue:
	uint32_t num_leaves = 0;
	uint32_t first_leaf = 0;
	uint32_t *leaf_queue = malloc(contour_tree->num_nodes * sizeof(uint32_t));
	if (!leaf_queue)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for leaf queue.");
		free(up_degrees);
		return -1;
	}
	for (uint32_t i = 0; i < contour_tree->num_nodes; i++)
	{
		up_degrees[i] = join_tree->nodes[i].degree[0];
		if ((join_tree->nodes[i].degree[0] + split_tree->nodes[i].degree[1]) == 1)
		{
			leaf_queue[num_leaves] = i;
			num_leaves++;
		}
	}

	// Merge join and split trees:
	uint32_t leaf;
	uint32_t node;
	uint32_t higher_node;
	uint32_t lower_node;
	while (num_leaves > 1)
	{
		leaf = leaf_queue[first_leaf];
		first_leaf++;
		num_leaves--;

		// Meshes with multiple disconnected components need this:
		if (!join_tree->nodes[leaf].degree[0] && !split_tree->nodes[leaf].degree[1])
		{
			continue;
		}

		if (!join_tree->nodes[leaf].degree[0]) // Upper leaf.
		{
			// Add arc to contour tree:
			node = join_tree->arcs[join_tree->nodes[leaf].first_arc];

			contour_tree->arcs[contour_tree->nodes[leaf].first_arc +
				up_degrees[leaf] + contour_tree->nodes[leaf].degree[1]] = node;
			contour_tree->nodes[leaf].degree[1]++;

			contour_tree->arcs[contour_tree->nodes[node].first_arc +
				contour_tree->nodes[node].degree[0]] = leaf;
			contour_tree->nodes[node].degree[0]++;

			contour_tree->num_arcs++;

			// Remove arc from join tree:
			for (uint32_t i = 0; i < join_tree->nodes[node].degree[0]; i++)
			{
				if (join_tree->arcs[join_tree->nodes[node].first_arc + i] == leaf)
				{
					join_tree->arcs[join_tree->nodes[node].first_arc + i] =
						join_tree->arcs[join_tree->nodes[node].first_arc +
							join_tree->nodes[node].degree[0] - 1];
					join_tree->arcs[join_tree->nodes[node].first_arc +
						join_tree->nodes[node].degree[0] - 1] =
						join_tree->arcs[join_tree->nodes[node].first_arc +
						join_tree->nodes[node].degree[0] +
						join_tree->nodes[node].degree[1] - 1];
					break;
				}
			}
			join_tree->nodes[node].degree[0]--;

			// Remove arc from split tree:
			lower_node = split_tree->arcs[split_tree->nodes[leaf].first_arc +
						split_tree->nodes[leaf].degree[0]];
			if (split_tree->nodes[leaf].degree[0])
			{
				// Join higher node to lower one:
				higher_node = split_tree->arcs[split_tree->nodes[leaf].first_arc];
				split_tree->arcs[split_tree->nodes[higher_node].first_arc +
					split_tree->nodes[higher_node].degree[0]] = lower_node;
				split_tree->arcs[split_tree->nodes[lower_node].first_arc] =
									higher_node;
			}
			else if (split_tree->nodes[lower_node].degree[0])
			{
				split_tree->nodes[lower_node].degree[0]--;
			}
		}
		else // Lower leaf.
		{
			// Add arc to contour tree:
			node = split_tree->arcs[split_tree->nodes[leaf].first_arc];

			contour_tree->arcs[contour_tree->nodes[leaf].first_arc +
				contour_tree->nodes[leaf].degree[0]] = node;
			contour_tree->nodes[leaf].degree[0]++;

			contour_tree->arcs[contour_tree->nodes[node].first_arc +
				up_degrees[node] + contour_tree->nodes[node].degree[1]] = leaf;
			contour_tree->nodes[node].degree[1]++;

			contour_tree->num_arcs++;

			// Remove arc from split tree:
			for (uint32_t i = split_tree->nodes[node].degree[0];
				i < (split_tree->nodes[node].degree[0] +
				split_tree->nodes[node].degree[1]); i++)
			{
				if (split_tree->arcs[split_tree->nodes[node].first_arc + i] == leaf)
				{
					split_tree->arcs[split_tree->nodes[node].first_arc + i] =
						split_tree->arcs[split_tree->nodes[node].first_arc +
							split_tree->nodes[node].degree[0] +
							split_tree->nodes[node].degree[1] - 1];
					break;
				}
			}
			split_tree->nodes[node].degree[1]--;

			// Remove arc from join tree:
			higher_node = join_tree->arcs[join_tree->nodes[leaf].first_arc];
			if (join_tree->nodes[leaf].degree[1])
			{
				// Join higher node to lower one:
				lower_node = join_tree->arcs[join_tree->nodes[leaf].first_arc +
							split_tree->nodes[leaf].degree[1]];
				join_tree->arcs[join_tree->nodes[higher_node].first_arc +
					join_tree->nodes[higher_node].degree[0]] = lower_node;
				join_tree->arcs[join_tree->nodes[lower_node].first_arc] =
									higher_node;
			}
			else if (join_tree->nodes[higher_node].degree[1])
			{
				join_tree->nodes[higher_node].degree[1]--;
			}
		}

		// Nullify leaf:
		join_tree->nodes[leaf].degree[0] = join_tree->nodes[leaf].degree[1] = 0;
		split_tree->nodes[leaf].degree[0] = split_tree->nodes[leaf].degree[1] = 0;

		// Check if connected node is now a leaf:
		if ((join_tree->nodes[node].degree[0] + split_tree->nodes[node].degree[1]) == 1)
		{
			leaf_queue[first_leaf + num_leaves] = node;
			num_leaves++;
		}
	}

	free(leaf_queue);
	free(up_degrees);
	return 0;
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

/********************
 * Scalar functions *
 ********************/

int ct_tree_scalar_function_y(ct_tree_t *tree, ct_mesh_t *mesh,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	if (ct_mesh_check_validity(mesh, error_message)) { return -1; }

	ct_tree_free(tree);
	tree->num_nodes = mesh->num_vertices;
	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	if (!tree->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for tree nodes.");
		return -1;
	}
	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[i].node_to_vertex = i;
		tree->nodes[i].value = mesh->vertices[i].y;
	}

	ct_tree_sort_nodes(tree);

	return 0;
}

int ct_tree_scalar_function_z(ct_tree_t *tree, ct_mesh_t *mesh,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	if (ct_mesh_check_validity(mesh, error_message)) { return -1; }

	ct_tree_free(tree);
	tree->num_nodes = mesh->num_vertices;
	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	if (!tree->nodes)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for tree nodes.");
		return -1;
	}
	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[i].node_to_vertex = i;
		tree->nodes[i].value = mesh->vertices[i].z;
	}

	ct_tree_sort_nodes(tree);

	return 0;
}

/*****************
 * Disjoint sets *
 *****************/

int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set,
		char error_message[NM_MAX_ERROR_LENGTH])
{
	disjoint_set->parent = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->rank = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->extremum = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	if (!disjoint_set->parent || !disjoint_set->rank || !disjoint_set->extremum)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for disjoint set.");
		return -1;
	}

	for (uint32_t i = 0; i < disjoint_set->num_elements; i++)
	{
		disjoint_set->parent[i] = i;
		disjoint_set->rank[i] = 0;
		disjoint_set->extremum[i] = i;
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

	if (disjoint_set->extremum)
	{
		free(disjoint_set->extremum);
		disjoint_set->extremum = NULL;
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
void ct_tree_print_short(FILE *file, ct_tree_t *tree)
{
	fprintf(file, "%u Node", tree->num_nodes);
	if (tree->num_nodes != 1) { fprintf(file, "s"); }
	fprintf(file, "\n");

	fprintf(file, "%u Arc", tree->num_arcs);
	if (tree->num_arcs != 1) { fprintf(file, "s"); }
	fprintf(file, "\n");

	fprintf(file, "%u Root", tree->num_roots);
	if (tree->num_roots != 1) { fprintf(file, "s"); }
	fprintf(file, "\n");
}

void ct_tree_print(FILE *file, ct_tree_t *tree)
{
	fprintf(file, "Note: printing only critical arcs and nodes.\n");
	fprintf(file, "\n%u Node", tree->num_nodes);
	if (tree->num_nodes != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		if (!ct_tree_node_is_critical(&(tree->nodes[i]))) { continue; }
		fprintf(file, "Node %u: %u -> Up = %u, Down = %u\n", i,
			tree->nodes[i].node_to_vertex, tree->nodes[i].degree[0],
			tree->nodes[i].degree[1]);
	}

	fprintf(file, "\n%u Arc", tree->num_arcs);
	if (tree->num_arcs != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	uint32_t arcs_printed = 0;
	uint32_t first_arc = 0;
	uint32_t num_arcs = 0;
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		first_arc = tree->nodes[i].first_arc;
		num_arcs = tree->nodes[i].degree[0];
		for (uint32_t j = first_arc; j < (first_arc + num_arcs); j++)
		{
			if (!ct_tree_node_is_critical(&(tree->nodes[i])) &&
				!ct_tree_node_is_critical(&(tree->nodes[tree->arcs[j]])))
			{
				continue;
			}
			fprintf(file, "Arc %u: %u --> %u\n", arcs_printed, i, tree->arcs[j]);
			arcs_printed++;
		}
	}

	fprintf(file, "\n%u Root", tree->num_roots);
	if (tree->num_roots != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	if (tree->num_roots) { fprintf(file, "%u", tree->roots[0]); }
	for (uint32_t i = 1; i < tree->num_roots; i++)
	{
		fprintf(file, ", %u", tree->roots[i]);
	}
	if (tree->num_roots) { fprintf(file, "\n"); }
}

int ct_tree_build_test_case(ct_tree_t *join_tree, ct_tree_t *split_tree,
						char error_message[NM_MAX_ERROR_LENGTH])
{
	float node_labels[18] = { 1.f, 2.f, 2.1f, 3.f, 4.f, 4.6f,
				4.9f, 5.f, 6.f, 6.1f, 6.5f, 6.9f,
				7.f, 7.2f, 8.f, 8.3f, 9.f, 10.f };

	join_tree->num_nodes	= split_tree->num_nodes	= 18;
	join_tree->num_arcs	= split_tree->num_arcs	= 17;
	join_tree->num_roots	= split_tree->num_roots	= 1;

	join_tree->nodes = malloc(join_tree->num_nodes * sizeof(ct_tree_node_t));
	join_tree->arcs = malloc(join_tree->num_arcs * 2 * sizeof(uint32_t));
	join_tree->roots = malloc(join_tree->num_roots * sizeof(uint32_t));

	split_tree->nodes = malloc(split_tree->num_nodes * sizeof(ct_tree_node_t));
	split_tree->arcs = malloc(split_tree->num_arcs * 2 * sizeof(uint32_t));
	split_tree->roots = malloc(split_tree->num_roots * sizeof(uint32_t));

	if (!join_tree->nodes || !join_tree->arcs || !join_tree->roots ||
		!split_tree->nodes || !split_tree->arcs || !split_tree->roots)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for test case join and split trees.");
		return -1;
	}

	uint32_t up_degrees_join[18] = { 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1, 0, 1, 0, 1, 0, 0 };
	uint32_t down_degrees_join[18] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	uint32_t up_degrees_split[18] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
	uint32_t down_degrees_split[18] = { 0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	uint32_t first_arcs_join[18] = { 0, 1, 3, 5, 7, 10, 12, 14, 17, 20,
					22, 24, 26, 27, 29, 30, 32, 33 };
	uint32_t first_arcs_split[18] = { 0, 1, 2, 4, 7, 9, 11, 13, 15, 17,
					19, 21, 23, 25, 27, 29, 31, 33 };

	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		join_tree->nodes[i].value = split_tree->nodes[i].value = node_labels[i];
		join_tree->nodes[i].node_to_vertex = split_tree->nodes[i].node_to_vertex = i;
		join_tree->nodes[i].vertex_to_node = split_tree->nodes[i].vertex_to_node = i;

		join_tree->nodes[i].degree[0] = up_degrees_join[i];
		join_tree->nodes[i].degree[1] = down_degrees_join[i];
		split_tree->nodes[i].degree[0] = up_degrees_split[i];
		split_tree->nodes[i].degree[1] = down_degrees_split[i];

		join_tree->nodes[i].first_arc = first_arcs_join[i];
		split_tree->nodes[i].first_arc = first_arcs_split[i];
	}

	uint32_t arcs_join[34] = { 1, 2, 0, 3, 1, 4, 2, 5, 6, 3, 7,
				4, 8, 4, 9, 10, 5, 11, 12, 6, 13, 7,
				16, 7, 14, 8, 8, 15, 9, 11, 17, 13, 10, 15 };

	uint32_t arcs_split[34] = { 2, 3, 3, 0, 4, 1, 2, 5, 3, 6, 4, 7,
				5, 8, 6, 9, 7, 10, 8, 11, 9, 12, 10, 13,
				11, 14, 12, 15, 13, 16, 14, 17, 15, 16 };

	memcpy(join_tree->arcs, arcs_join, 34 * sizeof(uint32_t));
	memcpy(split_tree->arcs, arcs_split, 34 * sizeof(uint32_t));

	join_tree->roots[0] = 0;
	split_tree->roots[0] = 17;

	return 0;
}

void ct_tree_print_test_case(FILE *file, ct_tree_t *tree)
{
	float node_labels[18] = { 1.f, 2.f, 2.1f, 3.f, 4.f, 4.6f, 4.9f, 5.f,
		6.f, 6.1f, 6.5f, 6.9f, 7.f, 7.2f, 8.f, 8.3f, 9.f, 10.f };

	fprintf(file, "\n%u Node", tree->num_nodes);
	if (tree->num_nodes != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		fprintf(file, "Node %.1f -> Up = %u, Down = %u", node_labels[i],
			tree->nodes[i].degree[0], tree->nodes[i].degree[1]);
		if (!tree->nodes[i].degree[0] && !tree->nodes[i].degree[1])
		{
			fprintf(file, "\t(deleted)");
		}
		fprintf(file, "\n");
	}

	fprintf(file, "\n%u Arc", tree->num_arcs);
	if (tree->num_arcs != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	uint32_t arcs_printed = 0;
	uint32_t first_arc = 0;
	uint32_t num_arcs = 0;
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		first_arc = tree->nodes[i].first_arc;
		num_arcs = tree->nodes[i].degree[0];
		for (uint32_t j = first_arc; j < (first_arc + num_arcs); j++)
		{
			fprintf(file, "Arc %u: %.1f --> %.1f\n", arcs_printed,
				node_labels[i], node_labels[tree->arcs[j]]);
			arcs_printed++;
		}
	}

	fprintf(file, "\n%u Root", tree->num_roots);
	if (tree->num_roots != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	if (tree->num_roots) { fprintf(file, "%.1f", node_labels[tree->roots[0]]); }
	for (uint32_t i = 1; i < tree->num_roots; i++)
	{
		fprintf(file, ", %.1f", node_labels[tree->roots[i]]);
	}
	if (tree->num_roots) { fprintf(file, "\n"); }
}
#endif
