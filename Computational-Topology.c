#include <stdio.h>
#include <string.h>
#include <time.h>

#include <NM-Config/Config.h>

#include "Source/Computational-Topology.h"
#include "Source/GUI.h"

void get_time(struct timespec *timer);
void print_time_end(FILE *file, struct timespec *start, char *message);

int main(int argc, char **argv)
{
	char error_message[NM_MAX_ERROR_LENGTH];
	strcpy(error_message, "");
	struct timespec start;
	ct_gui_t gui = {0};
	ct_mesh_t mesh = {0};
	ct_tree_t join_tree = {0};
	ct_tree_t split_tree = {0};
	ct_tree_t contour_tree = {0};

	// Get mesh filename:
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/spot.obj"); }
	strcpy(mesh.name, mesh.path);

	// GUI setup:
	if (ct_gui_setup(&gui)) { goto error; }

	// Mesh load:
	get_time(&start);
	if (ct_mesh_load(&mesh, error_message)) { goto error; }
	#ifdef CT_DEBUG
	fprintf(stdout, "\n");
	ct_mesh_print_short(stdout, &mesh);
	#endif
	print_time_end(stdout, &start, "(mesh load):\t\t");

	// Scalar value setup:
	get_time(&start);
	if (ct_tree_scalar_function_y(&join_tree, &mesh, error_message)) { goto error; }
	if (ct_tree_copy_nodes(&join_tree, &split_tree, error_message)) { goto error; }
	print_time_end(stdout, &start, "(vertex sorting):\t");

	// Merge tree computation:
	get_time(&start);

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

	fprintf(stdout, "\n");
	#endif

	print_time_end(stdout, &start, "(merge trees):\t");

	// Contour tree computation:
	get_time(&start);
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
	print_time_end(stdout, &start, "(contour tree):\t");

	// Main loop:
	SDL_Event event;
	int running = 1;
	while (running)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EVENT_QUIT) { running = 0; }
		}

		if (ct_gui_start_frame(&gui)) { break; }
		if (ct_gui_render(&gui)) { break; }
	}

	// Cleanup, success:
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_tree_free(&contour_tree);
	ct_mesh_free(&mesh);
	ct_gui_shutdown(&gui);
	return 0;

	// Cleanup, failure:
	error:
	if (!strcmp(gui.vulkan.error_message, ""))
	{
		fprintf(stdout, "Error: %s\n", gui.vulkan.error_message);
	}
	else { fprintf(stdout, "Error: %s\n", error_message); }
	ct_tree_free(&join_tree);
	ct_tree_free(&split_tree);
	ct_tree_free(&contour_tree);
	ct_mesh_free(&mesh);
	ct_gui_shutdown(&gui);
	return -1;
}

void get_time(struct timespec *timer)
{
	clock_gettime(CLOCK_MONOTONIC, timer);
}

void print_time_end(FILE *file, struct timespec *start, char *message)
{
	double total;
	struct timespec end;
	get_time(&end);
	total = (double)(end.tv_sec) + ((double)(end.tv_nsec) / 1000000000.f);
	total -= (double)(start->tv_sec) + ((double)(start->tv_nsec) / 1000000000.f);
	fprintf(file, "Time taken %s\t%f seconds\n", message, total);
}
