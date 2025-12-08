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
	ct_tree_t join_tree = {0};
	ct_tree_t split_tree = {0};
	ct_tree_t contour_tree = {0};

	struct timespec timer_start;
	struct timespec timer_end;
	double total_time;

	// Mesh load:
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/spot.obj"); }
	strcpy(mesh.name, mesh.path);

	clock_gettime(CLOCK_MONOTONIC, &timer_start);

	if (ct_mesh_load(&mesh, error_message)) { goto error; }

	#ifdef CT_DEBUG
	fprintf(stdout, "\n");
	ct_mesh_print_short(stdout, &mesh);
	#endif

	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "\nTime taken (mesh load):\t\t\t\t\t%f\t(s)\n", total_time);

	// Scalar value setup:
	clock_gettime(CLOCK_MONOTONIC, &timer_start);

	if (ct_tree_scalar_function_y(&join_tree, &mesh, error_message)) { goto error; }
	if (ct_tree_copy_nodes(&join_tree, &split_tree, error_message)) { goto error; }

	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "\nTime taken (vertex sorting):\t\t\t\t%f\t(s)\n", total_time);

	// Merge tree computation:
	clock_gettime(CLOCK_MONOTONIC, &timer_start);

	uint32_t start_index = join_tree.num_nodes - 1;
	if (ct_merge_tree_construct(&join_tree, &mesh, start_index, error_message))
	{
		goto error;
	}

	start_index = 0;
	if (ct_merge_tree_construct(&split_tree, &mesh, start_index, error_message))
	{
		goto error;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n*************\n");
	fprintf(stdout, "* Join tree *\n");
	fprintf(stdout, "*************\n");
	ct_tree_print_short(stdout, &join_tree);

	fprintf(stdout, "\n**************\n");
	fprintf(stdout, "* Split tree *\n");
	fprintf(stdout, "**************\n");
	ct_tree_print_short(stdout, &split_tree);
	#endif

	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "\nTime taken (merge trees):\t\t\t\t%f\t(s)\n", total_time);

	// Contour tree computation:
	clock_gettime(CLOCK_MONOTONIC, &timer_start);

	if (ct_contour_tree_construct(&contour_tree, &join_tree, &split_tree, error_message))
	{
		goto error;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n****************\n");
	fprintf(stdout, "* Contour tree *\n");
	fprintf(stdout, "****************\n");
	ct_tree_print_short(stdout, &contour_tree);
	#endif

	clock_gettime(CLOCK_MONOTONIC, &timer_end);
	total_time = (double)(timer_end.tv_sec) + ((double)(timer_end.tv_nsec) / 1000000000.f);
	total_time -= (double)(timer_start.tv_sec) + ((double)(timer_start.tv_nsec) / 1000000000.f);
	fprintf(stdout, "\nTime taken (contour tree):\t\t\t\t%f\t(s)\n", total_time);

	// Cleanup, success:
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_tree_free(&contour_tree);
	ct_mesh_free(&mesh);
	return 0;

	// Cleanup, failure:
	error:
	printf("Error: %s\n", error_message);
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_tree_free(&contour_tree);
	ct_mesh_free(&mesh);
	return -1;
}
