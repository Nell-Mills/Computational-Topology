#include "Mesh.h"

mp_mesh_t mp_mesh_initialise()
{
	mp_mesh_t mesh;
	memset(&mesh, 0, sizeof(mesh));

	strcpy(mesh.name, "");
	strcpy(mesh.path, "");

	mesh.vertices		= NULL;
	mesh.normals		= NULL;
	mesh.colours		= NULL;
	mesh.uv_coordinates	= NULL;

	mesh.edges		= NULL;
	mesh.first_edge		= NULL;

	mesh.num_lod_levels	= 1;
	for (int i = 0; i < NM_MAX_LOD_LEVELS; i++) { mesh.faces[i] = NULL; }

	return mesh;
}

int mp_mesh_allocate(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	mp_mesh_free(mesh);

	mesh->vertices = malloc(mesh->num_vertices * sizeof(mp_position_t));
	mesh->normals = malloc(mesh->num_normals * sizeof(mp_normal_t));
	mesh->colours = malloc(mesh->num_colours * sizeof(mp_colour_t));
	mesh->uv_coordinates = malloc(mesh->num_uv_coordinates * sizeof(mp_uv_t));
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

	mp_mesh_free_edges(mesh);

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
	mp_mesh_free_edges(mesh);

	mesh->num_edges = mesh->num_faces[0] * 3;
	mesh->edges = malloc(mesh->num_edges * sizeof(mp_edge_t));
	mesh->first_edge = malloc(mesh->num_vertices * sizeof(int64_t));
	if (!mesh->edges || !mesh->first_edge)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for edges of mesh \"%s\".", mesh->name);
		return -1;
	}
	memset(mesh->edges, 0, mesh->num_edges * sizeof(mp_edge_t));
	memset(mesh->first_edge, 0, mesh->num_vertices * sizeof(int64_t));

	#pragma omp parallel for default(shared)
	for (int64_t i = 0; i < mesh->num_faces[0]; i++)
	{
		mesh->edges[i * 3].from			= mesh->faces[0][i].p[0];
		mesh->edges[i * 3].to			= mesh->faces[0][i].p[1];
		mesh->edges[i * 3].next			= (i * 3) + 1;
		mesh->edges[i * 3].other_half		= -1;
		mesh->edges[i * 3].face			= i;

		mesh->edges[(i * 3) + 1].from		= mesh->faces[0][i].p[1];
		mesh->edges[(i * 3) + 1].to		= mesh->faces[0][i].p[2];
		mesh->edges[(i * 3) + 1].next		= (i * 3) + 2;
		mesh->edges[(i * 3) + 1].other_half	= -1;
		mesh->edges[(i * 3) + 1].face		= i;

		mesh->edges[(i * 3) + 2].from		= mesh->faces[0][i].p[2];
		mesh->edges[(i * 3) + 2].to		= mesh->faces[0][i].p[0];
		mesh->edges[(i * 3) + 2].next		= i * 3;
		mesh->edges[(i * 3) + 2].other_half	= -1;
		mesh->edges[(i * 3) + 2].face		= i;
	}

	for (int64_t i = 0; i < mesh->num_edges; i++)
	{
		if (mesh->edges[i].other_half != -1) { continue; }
		for (int j = (i + 1); j < mesh->num_edges; j++)
		{
			if ((mesh->edges[i].from == mesh->edges[j].to) &&
				(mesh->edges[i].to == mesh->edges[j].from))
			{
				mesh->edges[i].other_half = j;
				mesh->edges[j].other_half = i;
				break;
			}
		}
	}

	#pragma omp parallel for default(shared)
	for (uint16_t i = 0; i < mesh->num_vertices; i++)
	{
		mesh->first_edge[i] = -1;
		for (int64_t j = 0; j < mesh->num_edges; j++)
		{
			if (mesh->edges[j].from == i)
			{
				mesh->first_edge[i] = j;
				break;
			}
		}
	}

	return 0;
}

void mp_mesh_free_edges(mp_mesh_t *mesh)
{
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
}

#ifdef MP_DEBUG
void mp_mesh_print(FILE *file, mp_mesh_t *mesh)
{
	fprintf(file, "*******************\n");
	fprintf(file, "* Mesh debug info *\n");
	fprintf(file, "*******************\n");

	fprintf(file, "Mesh name: %s\n", mesh->name);
	fprintf(file, "Mesh path: %s\n", mesh->path);

	fprintf(file, "\n");

	fprintf(file, "Number of vertices: %u\n", mesh->num_vertices);
	fprintf(file, "Number of normals: %u\n", mesh->num_normals);
	fprintf(file, "Number of colours: %u\n", mesh->num_colours);
	fprintf(file, "Number of UV coordinates: %u\n", mesh->num_uv_coordinates);
	fprintf(file, "Number of edges: %ld\n", mesh->num_edges);
	fprintf(file, "Number of faces: ");
	for (int i = 0; i < (mesh->num_lod_levels - 1); i++)
	{
		fprintf(file, "%u, ", mesh->num_faces[i]);
	}
	fprintf(file, "%u\n", mesh->num_faces[mesh->num_lod_levels - 1]);

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
			if (mesh->first_edge)
			{
				fprintf(file, "--> First edge: %ld\n", mesh->first_edge[i]);
			}
		}

		if (mesh->edges)
		{
			fprintf(file, "\nData for edges in first face:\n");
			for (int i = 0; i < 3; i++)
			{
				fprintf(file, "Edge %d:\n", i);
				fprintf(file, "--> From vertex %u to %u\n", mesh->edges[i].from,
									mesh->edges[i].to);
				fprintf(file, "--> Next edge: %ld\n", mesh->edges[i].next);
				fprintf(file, "--> Other half: %ld\n", mesh->edges[i].other_half);
				fprintf(file, "--> Face: %u\n", mesh->edges[i].face);
				if (mesh->edges[i].other_half != -1)
				{
					fprintf(file, "--> Other face: %u\n",
						mesh->edges[mesh->edges[i].other_half].face);
				}
				else
				{
					fprintf(file, "--> Other face: N/A\n");
				}
			}
		}
	}
}
#endif
