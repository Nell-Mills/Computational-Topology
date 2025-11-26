#include "Mesh.h"

int mp_mesh_allocate(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	mp_mesh_free(mesh);
	mesh->num_lod_levels = 1;

	mesh->vertices = malloc(mesh->num_vertices * sizeof(mp_position_t));
	mesh->normals = malloc(mesh->num_normals * sizeof(mp_normal_t));
	mesh->colours = malloc(mesh->num_colours * sizeof(mp_colour_t));
	mesh->uv_coordinates = malloc(mesh->num_uv_coordinates * sizeof(mp_uv_t));
	mesh->edges = malloc(mesh->num_edges * sizeof(mp_edge_t));
	mesh->first_edge = malloc(mesh->num_vertices * sizeof(uint32_t));
	mesh->faces[0] = malloc(mesh->num_faces[0] * sizeof(mp_face_t));
	if (!mesh->vertices || !mesh->normals || !mesh->colours ||
			!mesh->uv_coordinates || !mesh->faces[0])
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for mesh \"%s\".", mesh->name);
		return -1;
	}

	memset(mesh->vertices, 0, mesh->num_vertices * sizeof(mp_position_t));
	memset(mesh->normals, 0, mesh->num_normals * sizeof(mp_normal_t));
	memset(mesh->colours, 0, mesh->num_colours * sizeof(mp_colour_t));
	memset(mesh->uv_coordinates, 0, mesh->num_uv_coordinates * sizeof(mp_uv_t));
	memset(mesh->edges, 0, mesh->num_edges * sizeof(mp_edge_t));
	memset(mesh->first_edge, 0, mesh->num_vertices * sizeof(uint32_t));
	memset(mesh->faces[0], 0, mesh->num_faces[0] * sizeof(mp_face_t));

	// Temporarily assign additional LOD levels to pointer for first:
	for (int i = 1; i < NM_MAX_LOD_LEVELS; i++) { mesh->faces[i] = mesh->faces[0]; }

	return 0;
}

void mp_mesh_free(mp_mesh_t *mesh)
{
	if (mesh->vertices)
	{
		free(mesh->vertices);
		mesh->vertices = NULL;
	}

	if (mesh->normals)
	{
		free(mesh->normals);
		mesh->normals = NULL;
	}

	if (mesh->colours)
	{
		free(mesh->colours);
		mesh->colours = NULL;
	}

	if (mesh->uv_coordinates)
	{
		free(mesh->uv_coordinates);
		mesh->uv_coordinates = NULL;
	}

	if (mesh->edges)
	{
		free(mesh->edges);
		mesh->edges = NULL;
	}

	if (mesh->first_edge)
	{
		free(mesh->first_edge);
		mesh->first_edge = NULL;
	}

	for (uint8_t i = 0; i < mesh->num_lod_levels; i++)
	{
		if (mesh->faces[i])
		{
			free(mesh->faces[i]);
			mesh->faces[i] = NULL;
		}
	}
}

int mp_mesh_calculate_edges(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	for (uint32_t i = 0; i < mesh->num_faces[0]; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			mesh->edges[(i * 3) + j].from		= mesh->faces[0][i].p[j];
			mesh->edges[(i * 3) + j].to		= mesh->faces[0][i].p[(j + 1) % 3];
			mesh->edges[(i * 3) + j].next		= (i * 3) + ((j + 1) % 3);
			mesh->edges[(i * 3) + j].other_half	= -1;

			mesh->first_edge[mesh->edges[(i * 3) + j].from] = (i * 3) + j;
		}
	}

	// Sort edges by lower vertex index, then higher:
	mp_edge_t *edges = malloc(mesh->num_edges * sizeof(mp_edge_t));
	if (!edges)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for edge sorting on mesh \"%s\".", mesh->name);
		return -1;
	}
	memcpy(edges, mesh->edges, mesh->num_edges * sizeof(mp_edge_t));
	qsort(edges, mesh->num_edges, sizeof(edges[0]), mp_mesh_edge_qsort_compare_low_high);

	// Other halves should now be adjacent:
	for (uint32_t i = 0; i < (mesh->num_edges - 1); i++)
	{
		if ((edges[i].from == edges[i + 1].to) &&
			(edges[i].to == edges[i + 1].from))
		{
			// Edges were sorted by face, so get original vertex from "next":
			uint32_t index_left = mp_mesh_get_edge_index(&(edges[i]));
			uint32_t index_right = mp_mesh_get_edge_index(&(edges[i + 1]));
			mesh->edges[index_left].other_half = index_right;
			mesh->edges[index_right].other_half = index_left;
		}
	}

	free(edges);
	return 0;
}

int mp_mesh_check_manifold(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	int return_value = 0;
	uint8_t is_manifold = 1;
	mp_edge_t *edges = NULL;
	uint32_t *vertex_degrees = NULL;

	edges = malloc(mesh->num_edges * sizeof(mp_edge_t));
	if (!edges)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for edge sorting on mesh \"%s\".", mesh->name);
		return_value = -1;
		goto cleanup;
	}
	memcpy(edges, mesh->edges, mesh->num_edges * sizeof(mp_edge_t));

	/***********************
	 * Edge duplicate test *
	 ***********************/

	// Sort edges by lower vertex index, then higher:
	qsort(edges, mesh->num_edges, sizeof(edges[0]), mp_mesh_edge_qsort_compare_low_high);

	#pragma omp parallel for shared(is_manifold)
	for (uint32_t i = 0; i < (mesh->num_edges - 1); i++)
	{
		if (!is_manifold) { continue; }
		if ((edges[i].from == edges[i + 1].from) && (edges[i].to == edges[i + 1].to))
		{
			is_manifold = 0;
		}
	}
	mesh->is_manifold = is_manifold;
	if (!mesh->is_manifold) { goto cleanup; }

	/*******************
	 * Edge pairs test *
	 *******************/

	/* This would be redundant, as the construction of "other halves" prevents mismatched or
	 * duplicated other half values. I'm leaving this here as a reminder in case, for some
	 * reason, you want to change the calculation of edges. */

	/**********************
	 * Vertex cycles test *
	 **********************/

	// Sort edges by "from":
	qsort(edges, mesh->num_edges, sizeof(edges[0]), mp_mesh_edge_qsort_compare_from);

	vertex_degrees = malloc(mesh->num_vertices * sizeof(uint32_t));
	if (!vertex_degrees)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for vertex degrees on mesh \"%s\".", mesh->name);
		return_value = -1;
		goto cleanup;
	}
	memset(vertex_degrees, 0, mesh->num_vertices * sizeof(uint32_t));

	uint32_t current_edge = 0;
	for (uint32_t i = 0; i < mesh->num_vertices; i++)
	{
		while(edges[current_edge].from == i)
		{
			vertex_degrees[i]++;
			current_edge++;
		}
	}

	#pragma omp parallel for shared(is_manifold)
	for (uint32_t i = 0; i < mesh->num_vertices; i++)
	{
		if (!is_manifold) { continue; }
		if (mp_mesh_triangle_fan_check(mesh, i, vertex_degrees[i])) { is_manifold = 0; }
	}
	mesh->is_manifold = is_manifold;

	cleanup:
	if (vertex_degrees) { free(vertex_degrees); }
	if (edges) { free(edges); }
	return return_value;
}

uint32_t mp_mesh_triangle_fan_check(mp_mesh_t *mesh, uint32_t vertex, uint32_t vertex_degree)
{
	uint32_t triangles_left = vertex_degree - 1; // Account for first triangle.

	int64_t current_edge = mesh->first_edge[vertex];
	while (1)
	{
		current_edge = mesh->edges[current_edge].next;
		if (mesh->edges[current_edge].to != vertex)
		{
			current_edge = mesh->edges[current_edge].next;
		}
		current_edge = mesh->edges[current_edge].other_half;
		if ((current_edge == mesh->first_edge[vertex]) || (current_edge == -1)) { break; }
		if (triangles_left == 0) { return vertex_degree; }
		triangles_left--;
	}

	if (current_edge == -1)
	{
		// Walk other way:
		current_edge = mesh->first_edge[vertex];
		while(1)
		{
			current_edge = mesh->edges[current_edge].other_half;
			if (current_edge == -1) { break; }
			current_edge = mesh->edges[current_edge].next;
			if (mesh->edges[current_edge].from != vertex)
			{
				current_edge = mesh->edges[current_edge].next;
			}
			if (triangles_left == 0) { return vertex_degree; }
			triangles_left--;
		}
	}

	return triangles_left;
}

uint32_t mp_mesh_get_edge_index(mp_edge_t *edge)
{
	if ((edge->next % 3) == 0) { return (edge->next + 2); }
	else { return (edge->next - 1); }
}

int mp_mesh_edge_qsort_compare_from(const void *a, const void *b)
{
	mp_edge_t left = *(mp_edge_t *)a;
	mp_edge_t right = *(mp_edge_t *)b;

	if (left.from < right.from) { return -1; }
	if (left.from > right.from) { return 1; }

	return 0;
}

int mp_mesh_edge_qsort_compare_low_high(const void *a, const void *b)
{
	mp_edge_t left = *(mp_edge_t *)a;
	mp_edge_t right = *(mp_edge_t *)b;

	uint32_t left_lower;
	uint32_t left_higher;
	uint32_t right_lower;
	uint32_t right_higher;

	if (left.from > left.to)
	{
		left_lower = left.to;
		left_higher = left.from;
	}
	else
	{
		left_lower = left.from;
		left_higher = left.to;
	}
	if (right.from > right.to)
	{
		right_lower = right.to;
		right_higher = right.from;
	}
	else
	{
		right_lower = right.from;
		right_higher = right.to;
	}

	if (left_lower < right_lower) { return -1; }
	if (left_lower > right_lower) { return 1; }
	if (left_higher < right_higher) { return -1; }
	if (left_higher > right_higher) { return 1; }

	return 0;
}

#ifdef MP_DEBUG
void mp_mesh_print_short(FILE *file, mp_mesh_t *mesh)
{
	fprintf(file, "*******************\n");
	fprintf(file, "* Mesh debug info *\n");
	fprintf(file, "*******************\n");

	fprintf(file, "Mesh name: %s\n", mesh->name);
	fprintf(file, "Mesh path: %s\n", mesh->path);
	fprintf(file, "Mesh is manifold: ");
	if (mesh->is_manifold) { fprintf(file, "Yes\n"); }
	else { fprintf(file, "No\n"); }

	fprintf(file, "\n");

	fprintf(file, "Number of vertices: %u\n", mesh->num_vertices);
	fprintf(file, "Number of normals: %u\n", mesh->num_normals);
	fprintf(file, "Number of colours: %u\n", mesh->num_colours);
	fprintf(file, "Number of UV coordinates: %u\n", mesh->num_uv_coordinates);
	fprintf(file, "Number of edges: %u\n", mesh->num_edges);
	fprintf(file, "Number of faces: %u", mesh->num_faces[0]);
	for (uint8_t i = 1; i < mesh->num_lod_levels; i++)
	{
		fprintf(file, ", %u", mesh->num_faces[i]);
	}
	fprintf(file, "\n");
}

void mp_mesh_print(FILE *file, mp_mesh_t *mesh)
{
	mp_mesh_print_short(file, mesh);

	if ((mesh->num_faces[0] > 0) && mesh->vertices && mesh->faces[0])
	{
		fprintf(file, "\n");

		fprintf(file, "Data for vertices in first face:\n");
		for (int i = 0; i < 3; i++)
		{
			fprintf(file, "Vertex %d:\n", i);
			fprintf(file, "--> Position %u: %f, %f, %f\n", mesh->faces[0][0].p[i],
				mesh->vertices[mesh->faces[0][0].p[i]].x,
				mesh->vertices[mesh->faces[0][0].p[i]].y,
				mesh->vertices[mesh->faces[0][0].p[i]].z);
			fprintf(file, "--> Normal %u: %d, %d, %d\n", mesh->faces[0][0].n[i],
				mesh->normals[mesh->faces[0][0].n[i]].x,
				mesh->normals[mesh->faces[0][0].n[i]].y,
				mesh->normals[mesh->faces[0][0].n[i]].z);
			fprintf(file, "--> Colour %u: %u, %u, %u, %u\n", mesh->faces[0][0].c[i],
				mesh->colours[mesh->faces[0][0].c[i]].r,
				mesh->colours[mesh->faces[0][0].c[i]].g,
				mesh->colours[mesh->faces[0][0].c[i]].b,
				mesh->colours[mesh->faces[0][0].c[i]].a);
			fprintf(file, "--> UV coordinates %u: %f, %f\n", mesh->faces[0][0].u[i],
				mesh->uv_coordinates[mesh->faces[0][0].u[i]].u,
				mesh->uv_coordinates[mesh->faces[0][0].u[i]].v);
			fprintf(file, "--> First edge: %d\n", mesh->first_edge[i]);
		}

		fprintf(file, "\nData for edges in first face:\n");
		for (int i = 0; i < 3; i++)
		{
			fprintf(file, "Edge %d:\n", i);
			fprintf(file, "--> From vertex %u to %u\n", mesh->edges[i].from,
								mesh->edges[i].to);
			fprintf(file, "--> Next edge: %u\n", mesh->edges[i].next);
			fprintf(file, "--> Face: %u\n", (i - (i % 3)) / 3);
			fprintf(file, "--> Other half: %ld\n", mesh->edges[i].other_half);
			if (mesh->edges[i].other_half != -1)
			{
				fprintf(file, "--> Other face: %ld\n", (mesh->edges[i].other_half -
							(mesh->edges[i].other_half % 3)) / 3);
			}
			else
			{
				fprintf(file, "--> Other face: N/A\n");
			}
		}
	}
}
#endif
