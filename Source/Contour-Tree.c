#include "Contour-Tree.h"

/*******************
 * Tree management *
 *******************/

void ct_tree_free(ct_tree_t *tree)
{
	if (tree->nodes) { free(tree->nodes); }
	if (tree->arcs) { free(tree->arcs); }
	if (tree->roots) { free(tree->roots); }
	memset(tree, 0, sizeof(*tree));
}

int8_t ct_tree_get_node_type(ct_tree_node_t *node)
{
	if (!node->degree[0] && !node->degree[1]) { return CT_NODE_TYPE_DELETED; }
	if (!node->degree[1]) { return CT_NODE_TYPE_MINIMUM; }
	if (!node->degree[0]) { return CT_NODE_TYPE_MAXIMUM; }
	if ((node->degree[0] > 1) || (node->degree[1] > 1)) { return CT_NODE_TYPE_SADDLE; }
	return CT_NODE_TYPE_REGULAR;
}

int ct_tree_node_is_critical(ct_tree_node_t *node)
{
	int8_t type = ct_tree_get_node_type(node);
	if ((type != CT_NODE_TYPE_DELETED) && (type != CT_NODE_TYPE_REGULAR)) { return 1; }
	return 0;
}

int ct_tree_copy_nodes(ct_tree_t *from, ct_tree_t *to, char error[NM_MAX_ERROR_LENGTH])
{
	ct_tree_free(to);
	to->num_nodes = from->num_nodes;
	to->nodes = malloc(to->num_nodes * sizeof(ct_tree_node_t));
	if (!to->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree nodes.");
		return -1;
	}
	memcpy(to->nodes, from->nodes, to->num_nodes * sizeof(ct_tree_node_t));

	return 0;
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
	uint32_t start_index, char error[NM_MAX_ERROR_LENGTH])
{
	if (!merge_tree->num_nodes || !merge_tree->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Merge tree has no nodes.");
		return -1;
	}

	ct_disjoint_set_t disjoint_set = {0};
	disjoint_set.num_elements = merge_tree->num_nodes;
	if (ct_disjoint_set_allocate(&disjoint_set, error)) { return -1; }

	merge_tree->arcs = malloc(merge_tree->num_nodes * 2 * sizeof(uint32_t));
	if (!merge_tree->arcs)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH,
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

	uint32_t current_arc[2] = { 0, merge_tree->num_nodes - 1 }; // Up[0], down[1].
	uint32_t current_edge;
	uint32_t current_vertex;
	uint32_t current_component;
	uint32_t adjacent_vertex;
	uint32_t adjacent_node;
	uint32_t adjacent_component;
	uint32_t previous_adjacent_vertex;
	while (1)
	{
		merge_tree->nodes[i].first_arc[!direction] = current_arc[!direction];
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
				merge_tree->arcs[current_arc[!direction]] = adjacent_node;
				merge_tree->arcs[current_arc[direction]] = i;
				merge_tree->nodes[adjacent_node].first_arc[direction] =
								current_arc[direction];
				current_arc[0]++;
				current_arc[1]++;
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
		if (!index_increment(&i, index_limit)) { break; }
	}

	// Get root information:
	merge_tree->roots = malloc(merge_tree->num_nodes * sizeof(uint32_t));
	if (!merge_tree->roots)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree roots.");
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
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not reallocate memory for tree roots.");
		ct_disjoint_set_free(&disjoint_set);
		return -1;
	}

	ct_disjoint_set_free(&disjoint_set);
	return 0;
}

int ct_merge_trees_reduce_to_critical(ct_tree_t *join_tree, ct_tree_t *split_tree,
						char error[NM_MAX_ERROR_LENGTH])
{
	ct_disjoint_set_t disjoint_set_join = {0};
	disjoint_set_join.num_elements = join_tree->num_nodes;
	if (ct_disjoint_set_allocate(&disjoint_set_join, error)) { return -1; }

	ct_disjoint_set_t disjoint_set_split = {0};
	disjoint_set_split.num_elements = split_tree->num_nodes;
	if (ct_disjoint_set_allocate(&disjoint_set_split, error))
	{
		ct_disjoint_set_free(&disjoint_set_join);
		return -1;
	}

	uint8_t *critical = malloc(join_tree->num_nodes * sizeof(uint8_t));
	if (!critical)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for critical node map.");
		ct_disjoint_set_free(&disjoint_set_split);
		ct_disjoint_set_free(&disjoint_set_join);
		return -1;
	}
	memset(critical, 0, join_tree->num_nodes * sizeof(uint8_t));

	uint32_t critical_count = 0;
	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		if (ct_tree_node_is_critical(&(join_tree->nodes[i])) ||
			ct_tree_node_is_critical(&(split_tree->nodes[i])))
		{
			critical[i] = 1;
			critical_count++;
		}
	}

	uint32_t index_split;
	uint32_t node_split = 0;
	uint32_t arc_split[2] = { 0, split_tree->num_arcs };

	uint32_t index_join;
	uint32_t node_join_final = critical_count - 1;
	uint32_t node_join = join_tree->num_nodes - 1;
	uint32_t arc_join[2] = { 0, join_tree->num_arcs };

	uint32_t component;
	uint32_t extremum;
	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		/*-----------*
		 - Join tree -
		 *-----------*/
		index_join = join_tree->num_nodes - i - 1;

		// Copy critical node info to appropriate space:
		if (critical[index_join])
		{
			join_tree->nodes[node_join].value = join_tree->nodes[index_join].value;
			join_tree->nodes[node_join].node_to_vertex =
				join_tree->nodes[index_join].node_to_vertex;
			join_tree->nodes[node_join].vertex_to_node =
				join_tree->nodes[index_join].vertex_to_node;
			join_tree->nodes[node_join].degree[0] =
				join_tree->nodes[index_join].degree[0];
			join_tree->nodes[node_join].degree[1] =
				join_tree->nodes[index_join].degree[1];
		}

		// Iterate through up arcs and keep track of components:
		for (uint32_t j = join_tree->nodes[index_join].first_arc[0];
			j < (join_tree->nodes[index_join].first_arc[0] +
			join_tree->nodes[index_join].degree[0]); j++)
		{
			extremum = disjoint_set_join.extremum[ct_disjoint_set_find(
					join_tree->arcs[j], &disjoint_set_join)];
			ct_disjoint_set_union(index_join, join_tree->arcs[j], &disjoint_set_join);
			component = ct_disjoint_set_find(join_tree->arcs[j], &disjoint_set_join);
			disjoint_set_join.extremum[component] = extremum;
			if (critical[index_join])
			{
				// Found superarc:
				join_tree->arcs[join_tree->nodes[extremum].first_arc[1]] =
									node_join_final;
				join_tree->arcs[arc_join[0]] = extremum - (join_tree->num_nodes - critical_count);
				arc_join[0]++;
			}
		}

		// Finalise critical node info:
		if (critical[index_join])
		{
			join_tree->nodes[node_join].first_arc[0] = arc_join[0] -
					join_tree->nodes[index_join].degree[0];
			join_tree->nodes[node_join].first_arc[1] = arc_join[1];

			component = ct_disjoint_set_find(index_join, &disjoint_set_join);
			disjoint_set_join.extremum[component] = node_join;
			arc_join[1] += join_tree->nodes[node_join].degree[1];
			node_join--;
			node_join_final--;
		}

		/*------------*
		 - Split tree -
		 *------------*/
		index_split = i;

		// Copy critical node info to appropriate space:
		if (critical[index_split])
		{
			split_tree->nodes[node_split].value = split_tree->nodes[index_split].value;
			split_tree->nodes[node_split].node_to_vertex =
				split_tree->nodes[index_split].node_to_vertex;
			split_tree->nodes[node_split].vertex_to_node =
				split_tree->nodes[index_split].vertex_to_node;
			split_tree->nodes[node_split].degree[0] =
				split_tree->nodes[index_split].degree[0];
			split_tree->nodes[node_split].degree[1] =
				split_tree->nodes[index_split].degree[1];

			component = ct_disjoint_set_find(index_split, &disjoint_set_split);
			disjoint_set_split.extremum[component] = node_split;
		}

		// Iterate through down arcs and keep track of components:
		for (uint32_t j = split_tree->nodes[index_split].first_arc[1];
			j < (split_tree->nodes[index_split].first_arc[1] +
			split_tree->nodes[index_split].degree[1]); j++)
		{
			extremum = disjoint_set_split.extremum[ct_disjoint_set_find(
					split_tree->arcs[j], &disjoint_set_split)];
			ct_disjoint_set_union(index_split, split_tree->arcs[j],
							&disjoint_set_split);
			component = ct_disjoint_set_find(split_tree->arcs[j], &disjoint_set_split);
			disjoint_set_split.extremum[component] = extremum;
			if (critical[index_split])
			{
				// Found superarc:
				split_tree->arcs[split_tree->nodes[extremum].first_arc[0]] =
										node_split;
				split_tree->arcs[arc_split[1]] = extremum;
				arc_split[1]++;
			}
		}

		// Finalise critical node info:
		if (critical[index_split])
		{
			split_tree->nodes[node_split].first_arc[1] = arc_split[1] -
					split_tree->nodes[index_split].degree[1];
			split_tree->nodes[node_split].first_arc[0] = arc_split[0];

			component = ct_disjoint_set_find(index_split, &disjoint_set_split);
			disjoint_set_split.extremum[component] = node_split;
			arc_split[0] += split_tree->nodes[node_split].degree[0];
			node_split++;
		}
	}

	free(critical);

	// Finalise new join tree information:
	join_tree->num_nodes = critical_count;
	if (!memmove(join_tree->nodes, &(join_tree->nodes[node_join + 1]), critical_count *
							sizeof(join_tree->nodes[0])))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not move join tree node memory.");
		ct_disjoint_set_free(&disjoint_set_split);
		ct_disjoint_set_free(&disjoint_set_join);
		return -1;
	}
	if (!memmove(&(join_tree->arcs[arc_join[0]]), &(join_tree->arcs[join_tree->num_arcs]),
		(join_tree->num_nodes - join_tree->num_roots) * sizeof(join_tree->arcs[0])))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not move join tree arc memory.");
		ct_disjoint_set_free(&disjoint_set_split);
		ct_disjoint_set_free(&disjoint_set_join);
		return -1;
	}
	join_tree->num_arcs = join_tree->num_nodes - join_tree->num_roots;

	// Finalise new split tree information:
	split_tree->num_nodes = critical_count;
	if (!memmove(&(split_tree->arcs[arc_split[0]]), &(split_tree->arcs[split_tree->num_arcs]),
		(split_tree->num_nodes - split_tree->num_roots) * sizeof(split_tree->arcs[0])))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not move split tree arc memory.");
		ct_disjoint_set_free(&disjoint_set_split);
		ct_disjoint_set_free(&disjoint_set_join);
		return -1;
	}
	split_tree->num_arcs = split_tree->num_nodes - split_tree->num_roots;

	// TODO realloc to smaller sizes.

	ct_disjoint_set_free(&disjoint_set_split);
	ct_disjoint_set_free(&disjoint_set_join);
	return 0;
}

int ct_contour_tree_construct(ct_tree_t *contour_tree, ct_tree_t *join_tree,
		ct_tree_t *split_tree, char error[NM_MAX_ERROR_LENGTH])
{
	/*****************
	 * Sanity checks *
	 *****************/

	if (!join_tree->nodes || !join_tree->arcs || !join_tree->roots)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Join tree is incomplete.");
		return -1;
	}
	if (!split_tree->nodes || !split_tree->arcs || !split_tree->roots)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Split tree is incomplete.");
		return -1;
	}
	if (join_tree->num_arcs != (join_tree->num_nodes - join_tree->num_roots))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Join tree has incorrect number of arcs.");
		return -1;
	}
	if (split_tree->num_arcs != (split_tree->num_nodes - split_tree->num_roots))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Split tree has incorrect number of arcs.");
		return -1;
	}
	if ((join_tree->num_nodes != split_tree->num_nodes) ||
		(join_tree->num_arcs != split_tree->num_arcs) ||
		(join_tree->num_roots != split_tree->num_roots))
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Join and split tree do not match.");
		return -1;
	}

	/***************************
	 * Contour tree node setup *
	 ***************************/

	contour_tree->num_nodes = join_tree->num_nodes;
	contour_tree->nodes = malloc(contour_tree->num_nodes * sizeof(ct_tree_node_t));
	if (!contour_tree->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree nodes.");
		return -1;
	}
	memset(contour_tree->nodes, 0, contour_tree->num_nodes * sizeof(ct_tree_node_t));

	contour_tree->nodes[0].first_arc[0] = 0;
	contour_tree->nodes[0].first_arc[1] = join_tree->num_arcs;
	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		contour_tree->nodes[i].value = join_tree->nodes[i].value;
		contour_tree->nodes[i].node_to_vertex = join_tree->nodes[i].node_to_vertex;

		// Degree information will be accumulated during merging.

		if (i > 0)
		{
			contour_tree->nodes[i].first_arc[0] =
				contour_tree->nodes[i - 1].first_arc[0] +
				join_tree->nodes[i - 1].degree[0];

			contour_tree->nodes[i].first_arc[1] =
				contour_tree->nodes[i - 1].first_arc[1] +
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
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree arcs.");
		return -1;
	}
	memset(contour_tree->arcs, 0, contour_tree->num_arcs * 2 * sizeof(uint32_t));
	contour_tree->num_arcs = 0;

	contour_tree->num_roots = join_tree->num_roots;
	contour_tree->roots = malloc(contour_tree->num_roots * sizeof(uint32_t));
	if (!contour_tree->roots)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for contour tree roots.");
		return -1;
	}
	memcpy(contour_tree->roots, join_tree->roots, contour_tree->num_roots * sizeof(uint32_t));

	/*****************************
	 * Contour tree construction *
	 *****************************/

	// Create leaf queue:
	uint32_t num_leaves = 0;
	uint32_t first_leaf = 0;
	uint32_t *leaf_queue = malloc(contour_tree->num_nodes * sizeof(uint32_t));
	if (!leaf_queue)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for leaf queue.");
		return -1;
	}
	for (uint32_t i = 0; i < contour_tree->num_nodes; i++)
	{
		if ((join_tree->nodes[i].degree[0] + split_tree->nodes[i].degree[1]) == 1)
		{
			leaf_queue[num_leaves] = i;
			num_leaves++;
		}
	}

	// Merge join and split trees:
	uint32_t leaf;
	uint32_t node;
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

		// Add arc to contour tree:
		if (!join_tree->nodes[leaf].degree[0]) // Upper leaf.
		{
			node = join_tree->arcs[join_tree->nodes[leaf].first_arc[1]];

			contour_tree->arcs[contour_tree->nodes[leaf].first_arc[1] +
				contour_tree->nodes[leaf].degree[1]] = node;
			contour_tree->nodes[leaf].degree[1]++;

			contour_tree->arcs[contour_tree->nodes[node].first_arc[0] +
				contour_tree->nodes[node].degree[0]] = leaf;
			contour_tree->nodes[node].degree[0]++;

			contour_tree->num_arcs++;
		}
		else // Lower leaf.
		{
			node = split_tree->arcs[split_tree->nodes[leaf].first_arc[0]];

			contour_tree->arcs[contour_tree->nodes[leaf].first_arc[0] +
				contour_tree->nodes[leaf].degree[0]] = node;
			contour_tree->nodes[leaf].degree[0]++;

			contour_tree->arcs[contour_tree->nodes[node].first_arc[1] +
				contour_tree->nodes[node].degree[1]] = leaf;
			contour_tree->nodes[node].degree[1]++;

			contour_tree->num_arcs++;
		}

		// Remove leaf from both trees:
		ct_tree_remove_node(join_tree, leaf);
		ct_tree_remove_node(split_tree, leaf);

		// Check if connected node is now a leaf:
		if ((join_tree->nodes[node].degree[0] + split_tree->nodes[node].degree[1]) == 1)
		{
			leaf_queue[first_leaf + num_leaves] = node;
			num_leaves++;
		}
	}

	free(leaf_queue);

	if (contour_tree->num_arcs != join_tree->num_arcs)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Contour tree has incorrect number of arcs. It has %u, and it should "
			"have %u.", contour_tree->num_arcs, join_tree->num_arcs);
		return -1;
	}

	return 0;
}

void ct_tree_remove_node(ct_tree_t *tree, uint32_t node)
{
	if ((tree->nodes[node].degree[0] > 1) ||
		(tree->nodes[node].degree[1] > 1))
	{
		#ifdef CT_DEBUG
		printf("Trying to remove a saddle: %u\n", node);
		#endif
		return;
	}
	if (!tree->nodes[node].degree[0] && !tree->nodes[node].degree[1])
	{
		#ifdef CT_DEBUG
		printf("Trying to remove a null node: %u\n", node);
		#endif
		return;
	}

	int direction = -1;
	if (!tree->nodes[node].degree[1])
	{
		// Lower leaf:
		direction = 0;
	}
	else if (!tree->nodes[node].degree[0])
	{
		// Upper leaf:
		direction = 1;
	}

	if (direction != -1)
	{
		// Process leaf node:
		uint32_t other = tree->arcs[tree->nodes[node].first_arc[direction]];
		int found = 0;
		for (uint32_t i = 0; i < tree->nodes[other].degree[!direction]; i++)
		{
			if (tree->arcs[tree->nodes[other].first_arc[!direction] + i] == node)
			{
				// Swap arc with last one:
				found = 1;
				uint32_t arc = tree->arcs[tree->nodes[other].first_arc[!direction] +
							tree->nodes[other].degree[!direction] - 1];
				tree->arcs[tree->nodes[other].first_arc[!direction] +
					tree->nodes[other].degree[!direction] - 1] = node;
				tree->arcs[tree->nodes[other].first_arc[!direction] + i] = arc;
				tree->nodes[other].degree[!direction]--;
				break;
			}
		}
		tree->nodes[node].degree[direction]--;
		#ifdef CT_DEBUG
		if (!found)
		{
			printf("\nLeaf not attached to anything: %u\n", node);
			printf("Node:\t%u,\tDegree: (%u, %u)\n", node,
				tree->nodes[node].degree[0], tree->nodes[node].degree[1]);
			printf("Other:\t%u,\tDegree: (%u, %u)\n", other,
				tree->nodes[other].degree[0], tree->nodes[other].degree[1]);
		}
		#endif
	}
	else
	{
		// Process regular node:
		uint32_t others[4]; // Higher node + arc, lower node + arc.
		for (int direction = 0; direction < 2; direction++)
		{
			uint32_t other = tree->arcs[tree->nodes[node].first_arc[direction]];
			uint32_t arc;
			others[direction * 2] = other;
			int found = 0;
			for (uint32_t i = 0; i < tree->nodes[other].degree[!direction]; i++)
			{
				if (tree->arcs[tree->nodes[other].first_arc[!direction] + i] == node)
				{
					found = 1;
					arc = tree->nodes[other].first_arc[!direction] + i;
					break;
				}
			}
			if (!found)
			{
				#ifdef CT_DEBUG
				printf("\nNode not attached to anything: %u\n", node);
				printf("Node:\t%u,\tDegree: (%u, %u)\n", node,
					tree->nodes[node].degree[0], tree->nodes[node].degree[1]);
				printf("Other:\t%u,\tDegree: (%u, %u)\n", other,
					tree->nodes[other].degree[0], tree->nodes[other].degree[1]);
				#endif
				return;
			}
			others[(direction * 2) + 1] = arc;
		}
		// Connect lower and higher nodes:
		tree->arcs[others[1]] = others[2];
		tree->arcs[others[3]] = others[0];
		tree->nodes[node].degree[0]--;
		tree->nodes[node].degree[1]--;
	}
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

int ct_tree_scalar_function_x(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH])
{
	if (ct_mesh_check_validity(mesh, error)) { return -1; }

	ct_tree_free(tree);
	tree->num_nodes = mesh->num_vertices;
	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	if (!tree->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree nodes.");
		return -1;
	}
	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[i].node_to_vertex = i;
		tree->nodes[i].value = mesh->vertices[i].x;
	}

	qsort(tree->nodes, tree->num_nodes, sizeof(tree->nodes[0]), ct_tree_nodes_qsort_compare);

	// Create 2-way mapping between vertices and nodes:
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[tree->nodes[i].node_to_vertex].vertex_to_node = i;
	}

	return 0;
}

int ct_tree_scalar_function_y(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH])
{
	if (ct_mesh_check_validity(mesh, error)) { return -1; }

	ct_tree_free(tree);
	tree->num_nodes = mesh->num_vertices;
	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	if (!tree->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree nodes.");
		return -1;
	}
	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[i].node_to_vertex = i;
		tree->nodes[i].value = mesh->vertices[i].y;
	}

	qsort(tree->nodes, tree->num_nodes, sizeof(tree->nodes[0]), ct_tree_nodes_qsort_compare);

	// Create 2-way mapping between vertices and nodes:
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[tree->nodes[i].node_to_vertex].vertex_to_node = i;
	}

	return 0;
}

int ct_tree_scalar_function_z(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH])
{
	if (ct_mesh_check_validity(mesh, error)) { return -1; }

	ct_tree_free(tree);
	tree->num_nodes = mesh->num_vertices;
	tree->nodes = malloc(tree->num_nodes * sizeof(ct_tree_node_t));
	if (!tree->nodes)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for tree nodes.");
		return -1;
	}
	memset(tree->nodes, 0, tree->num_nodes * sizeof(ct_tree_node_t));

	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[i].node_to_vertex = i;
		tree->nodes[i].value = mesh->vertices[i].z;
	}

	qsort(tree->nodes, tree->num_nodes, sizeof(tree->nodes[0]), ct_tree_nodes_qsort_compare);

	// Create 2-way mapping between vertices and nodes:
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		tree->nodes[tree->nodes[i].node_to_vertex].vertex_to_node = i;
	}

	return 0;
}

/*****************
 * Disjoint sets *
 *****************/

int ct_disjoint_set_allocate(ct_disjoint_set_t *disjoint_set, char error[NM_MAX_ERROR_LENGTH])
{
	disjoint_set->parent = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->rank = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	disjoint_set->extremum = malloc(disjoint_set->num_elements * sizeof(uint32_t));
	if (!disjoint_set->parent || !disjoint_set->rank || !disjoint_set->extremum)
	{
		snprintf(error, NM_MAX_ERROR_LENGTH, "Could not allocate memory for disjoint set.");
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
		first_arc = tree->nodes[i].first_arc[0];
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
				char error[NM_MAX_ERROR_LENGTH])
{
	ct_tree_free(join_tree);
	ct_tree_free(split_tree);

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
		snprintf(error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for test case join and split trees.");
		return -1;
	}

	uint32_t up_degrees_join[18] = { 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1, 0, 1, 0, 1, 0, 0 };
	uint32_t down_degrees_join[18] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	uint32_t up_degrees_split[18] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
	uint32_t down_degrees_split[18] = { 0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	join_tree->nodes[0].first_arc[0] = split_tree->nodes[0].first_arc[0] = 0;
	join_tree->nodes[0].first_arc[1] = split_tree->nodes[0].first_arc[1] = join_tree->num_arcs;
	for (uint32_t i = 0; i < join_tree->num_nodes; i++)
	{
		join_tree->nodes[i].value = split_tree->nodes[i].value = node_labels[i];
		join_tree->nodes[i].node_to_vertex = split_tree->nodes[i].node_to_vertex = i;

		join_tree->nodes[i].degree[0] = up_degrees_join[i];
		join_tree->nodes[i].degree[1] = down_degrees_join[i];
		split_tree->nodes[i].degree[0] = up_degrees_split[i];
		split_tree->nodes[i].degree[1] = down_degrees_split[i];

		if (i > 0)
		{
			join_tree->nodes[i].first_arc[0] = join_tree->nodes[i - 1].first_arc[0] +
							join_tree->nodes[i - 1].degree[0];
			join_tree->nodes[i].first_arc[1] = join_tree->nodes[i - 1].first_arc[1] +
							join_tree->nodes[i - 1].degree[1];
			split_tree->nodes[i].first_arc[0] = split_tree->nodes[i - 1].first_arc[0] +
							split_tree->nodes[i - 1].degree[0];
			split_tree->nodes[i].first_arc[1] = split_tree->nodes[i - 1].first_arc[1] +
							split_tree->nodes[i - 1].degree[1];
		}
	}

	uint32_t arcs_join[34] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 14, 15, 17,
				0, 1, 2, 3, 4, 4, 5, 6, 7, 7, 8, 8, 9, 11, 13, 10, 15 };

	uint32_t arcs_split[34] = { 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
				0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

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

	uint32_t arcs_printed = 0;
	uint32_t first_arc = 0;
	uint32_t num_arcs = 0;

	fprintf(file, "\n%u Up arc", tree->num_arcs);
	if (tree->num_arcs != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		first_arc = tree->nodes[i].first_arc[0];
		num_arcs = tree->nodes[i].degree[0];
		for (uint32_t j = first_arc; j < (first_arc + num_arcs); j++)
		{
			fprintf(file, "Arc %u: %.1f --> %.1f\n", arcs_printed,
				node_labels[i], node_labels[tree->arcs[j]]);
			arcs_printed++;
		}
	}

	fprintf(file, "\n%u Down arc", tree->num_arcs);
	if (tree->num_arcs != 1) { fprintf(file, "s"); }
	fprintf(file, ":\n");
	for (uint32_t i = 0; i < tree->num_nodes; i++)
	{
		first_arc = tree->nodes[i].first_arc[1];
		num_arcs = tree->nodes[i].degree[1];
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
