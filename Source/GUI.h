#ifndef CT_GUI_H
#define CT_GUI_H

#include <string.h>

#include <NM-Config/Config.h>
#include <Vulkan-Abstraction/Vulkan-Abstraction.h>

#include "Contour-Tree.h"
#include "Mesh.h"

typedef struct
{
	vka_vulkan_t vulkan;
	vka_command_buffer_t command_buffer;

	vka_pipeline_t mesh_pipeline;
	vka_pipeline_t tree_pipeline;
	vka_pipeline_t gui_pipeline;

	vka_render_info_t mesh_render_info;
	vka_render_info_t tree_render_info;
	vka_render_info_t gui_render_info;

	// TODO Nuklear GUI things.

	vka_vertex_buffers_t object_buffers;
	vka_vertex_buffers_t join_tree_buffers;
	vka_vertex_buffers_t split_tree_buffers;
	vka_vertex_buffers_t contour_tree_buffers;
} ct_gui_t;

int ct_gui_setup(ct_gui_t *gui);
int ct_gui_object_setup(ct_gui_t *gui, ct_mesh_t *mesh, ct_tree_t *join_tree,
			ct_tree_t *split_tree, ct_tree_t *contour_tree);
void ct_gui_shutdown(ct_gui_t *gui);

void ct_gui_update_window_size(ct_gui_t *gui);
int ct_gui_start_frame(ct_gui_t *gui);
int ct_gui_render(ct_gui_t *gui);

#endif
