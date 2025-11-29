#include <time.h>

#include <NM-Config/Config.h>
#include <Mesh-Processing/Mesh-Processing.h>

#include "Source/Computational-Topology.h"

int main(int argc, char **argv)
{
	char error_message[NM_MAX_ERROR_LENGTH];
	strcpy(error_message, "");
	mp_mesh_t mesh = {0};
	ct_contour_tree_t contour_tree = {0};
	ct_vertex_value_t *vertex_values = NULL;

	struct timespec timer_start;
	struct timespec timer_end;
	double total_time;

	// Mesh load:
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/spot.obj"); }
	strcpy(mesh.name, mesh.path);

	clock_gettime(CLOCK_MONOTONIC, &timer_start);
	if (mp_mesh_load(&mesh, error_message)) { goto error; }
	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "\nTime taken (mesh load):\t\t\t\t\t%f\t(s)\n", total_time);

	if (!mesh.is_manifold)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" is not manifold. Exiting.\n", mesh.name);
		goto error;
	}

	// Scalar value setup:
	vertex_values = malloc(mesh.num_vertices * sizeof(ct_vertex_value_t));
	if (!vertex_values) { goto error; }
	for (uint32_t i = 0; i < mesh.num_vertices; i++)
	{
		vertex_values[i].value = mesh.vertices[i].y;
		vertex_values[i].vertex = i;
	}

	clock_gettime(CLOCK_MONOTONIC, &timer_start);
	ct_vertex_values_sort(mesh.num_vertices, vertex_values);
	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "Time taken (vertex sorting):\t\t\t\t%f\t(s)\n", total_time);

	// Join tree computation:
	clock_gettime(CLOCK_MONOTONIC, &timer_start);
	if (ct_contour_tree_construct(&contour_tree, &mesh, vertex_values, error_message))
	{
		goto error;
	}
	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "Time taken (contour tree):\t\t\t\t%f\t(s)\n", total_time);

	#ifdef MP_DEBUG
	fprintf(stdout, "\n");
	ct_vertex_values_print(stdout, mesh.num_vertices, vertex_values);
	fprintf(stdout, "\n\n");
	mp_mesh_print_short(stdout, &mesh);
	#endif

	// Cleanup, success:
	free(vertex_values);
	ct_contour_tree_free(&contour_tree);
	mp_mesh_free(&mesh);
	return 0;

	// Cleanup, failure:
	error:
	printf("Error: %s\n", error_message);
	if (vertex_values) { free(vertex_values); }
	ct_contour_tree_free(&contour_tree);
	mp_mesh_free(&mesh);
	return -1;
}
