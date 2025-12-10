#ifndef CT_PROGRAM_H
#define CT_PROGRAM_H

#include <string.h>

#include <NM-Config/Config.h>
#include <Vulkan-Abstraction/Vulkan-Abstraction.h>

#include "Contour-Tree.h"
#include "Mesh.h"
#include "Mesh-Loader.h"

typedef struct
{
	vka_vulkan_t vulkan;
	vka_command_buffer_t command_buffer;
	vka_render_info_t render_info;

	// TODO Nuklear GUI things.

	vka_pipeline_t mesh_pipeline;
	vka_allocation_t buffer_allocation;
	vka_buffer_t index_buffer;
	vka_buffer_t vertex_buffers[3];

	int (*scalar_function)(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
	ct_mesh_t mesh;
	ct_tree_t join_tree;
	ct_tree_t split_tree;
	ct_tree_t contour_tree;

	char error[NM_MAX_ERROR_LENGTH];
} ct_program_t;

int ct_program_setup(ct_program_t *program);
void ct_program_shutdown(ct_program_t *program);

int ct_program_object_setup(ct_program_t *program);
void ct_program_object_shutdown(ct_program_t *program);

void ct_program_update_window_size(ct_program_t *program);
int ct_program_start_frame(ct_program_t *program);
int ct_program_render(ct_program_t *program);

#endif
