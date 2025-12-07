#include <stdio.h>
#include <string.h>
#include <time.h>

#include <NM-Config/Config.h>

#include "Source/Computational-Topology.h"
#include "Source/GUI.h"

int main(int argc, char **argv)
{
	char error_message[NM_MAX_ERROR_LENGTH];
	strcpy(error_message, "");
	ct_mesh_t mesh = {0};
	ct_vertex_value_t *vertex_values = NULL;
	ct_tree_t contour_tree = {0};
	ct_tree_t_NEW join_tree_NEW = {0};
	ct_tree_t_NEW split_tree_NEW = {0};
	ct_tree_t_NEW contour_tree_NEW = {0};

	struct timespec timer_start;
	struct timespec timer_end;
	double total_time;

	// Mesh load:
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/spot.obj"); }
	strcpy(mesh.name, mesh.path);

	clock_gettime(CLOCK_MONOTONIC, &timer_start);
	if (ct_mesh_load(&mesh, error_message)) { goto error; }
	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	//fprintf(stdout, "\nTime taken (mesh load):\t\t\t\t\t%f\t(s)\n", total_time);

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
	//fprintf(stdout, "Time taken (vertex sorting):\t\t\t\t%f\t(s)\n", total_time);

	#ifdef CT_DEBUG
	//fprintf(stdout, "\n");
	//ct_mesh_print_short(stdout, &mesh);
	#endif

	// Contour tree computation:
	clock_gettime(CLOCK_MONOTONIC, &timer_start);
	if (ct_contour_tree_construct(&contour_tree, &mesh, vertex_values, error_message))
	{
		goto error;
	}
	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	//fprintf(stdout, "Time taken (contour tree):\t\t\t\t%f\t(s)\n", total_time);

	#ifdef CT_DEBUG
	if (ct_tree_build_test_case_NEW(&join_tree_NEW, &split_tree_NEW, error_message))
	{
		goto error;
	}
	#else
	if (ct_tree_scalar_function_y_NEW(&join_tree_NEW, &mesh, error_message)) { goto error; }
	if (ct_tree_copy_nodes_NEW(&join_tree_NEW, &split_tree_NEW, error_message)) { goto error; }

	uint32_t start_index = join_tree_NEW.num_nodes - 1;
	if (ct_merge_tree_construct_NEW(&join_tree_NEW, &mesh, start_index, error_message))
	{
		goto error;
	}

	start_index = 0;
	if (ct_merge_tree_construct_NEW(&split_tree_NEW, &mesh, start_index, error_message))
	{
		goto error;
	}
	#endif

	if (ct_contour_tree_construct_NEW(&contour_tree_NEW, &join_tree_NEW, &split_tree_NEW,
									error_message))
	{
		goto error;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n*************\n");
	fprintf(stdout, "* Join tree *\n");
	fprintf(stdout, "*************\n");
	ct_tree_print_test_case_NEW(stdout, &join_tree_NEW);
	fprintf(stdout, "\n\n");
	fprintf(stdout, "\n**************\n");
	fprintf(stdout, "* Split tree *\n");
	fprintf(stdout, "**************\n");
	ct_tree_print_test_case_NEW(stdout, &split_tree_NEW);
	fprintf(stdout, "\n\n");
	fprintf(stdout, "\n****************\n");
	fprintf(stdout, "* Contour tree *\n");
	fprintf(stdout, "****************\n");
	ct_tree_print_test_case_NEW(stdout, &contour_tree_NEW);
	#endif

	// Cleanup, success:
	ct_tree_free_NEW(&join_tree_NEW);
	ct_tree_free_NEW(&split_tree_NEW);
	ct_tree_free_NEW(&contour_tree_NEW);
	ct_tree_free(&contour_tree);
	free(vertex_values);
	ct_mesh_free(&mesh);
	return 0;

	// Cleanup, failure:
	error:
	printf("Error: %s\n", error_message);
	ct_tree_free_NEW(&join_tree_NEW);
	ct_tree_free_NEW(&split_tree_NEW);
	ct_tree_free_NEW(&contour_tree_NEW);
	ct_tree_free(&contour_tree);
	if (vertex_values) { free(vertex_values); }
	ct_mesh_free(&mesh);
	return -1;
}
