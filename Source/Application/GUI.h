#ifndef CT_GUI_H
#define CT_GUI_H

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <NM-Config/Config.h>
#include <Vulkan-Abstraction/Vulkan-Abstraction.h>
#include <CGLM/cglm.h>

#include "../Core/Core.h"

#define CT_CLIP_NEAR	    0.1f
#define CT_CLIP_FAR	10000.0f

typedef struct
{
	mat4 model;
	mat4 view;
	mat4 projection;
	float isovalue;
	float max_value;
	float min_value;
	int discard_above;
	int discard_below;
	int highlight_equal;
	float highlight_size;
} ct_scene_uniform_t;

typedef struct
{
	vka_vulkan_t vulkan;
	vka_command_buffer_t command_buffer;
	vka_descriptor_pool_t descriptor_pool;

	// TODO Nuklear GUI things.

	vka_render_info_t render_info;
	vka_pipeline_t node_pipeline;
	vka_pipeline_t arc_pipeline;
	vka_pipeline_t mesh_pipeline;

	uint8_t update_scene_uniform;
	ct_scene_uniform_t scene_uniform;
	vka_allocation_t scene_uniform_allocation;
	vka_descriptor_set_t scene_uniform_descriptor_set;
	vka_buffer_t scene_uniform_buffer;

	// Sphere for nodes, cylinder for arcs:
	vka_allocation_t tree_mesh_allocation;
	vka_buffer_t sphere_buffer_index;
	vka_buffer_t sphere_buffer_positions;
	vka_buffer_t sphere_buffer_normals;
	vka_buffer_t cylinder_buffer_index;
	vka_buffer_t cylinder_buffer_positions;
	vka_buffer_t cylinder_buffer_normals;

	// Per-node info buffers:
	vka_allocation_t node_allocation;
	vka_descriptor_set_t node_descriptor_set;
	vka_buffer_t node_buffer_positions_scalars; // Both vertex buffer and storage buffer.
	vka_buffer_t node_buffer_index;
	vka_buffer_t join_node_buffer_types;
	vka_buffer_t split_node_buffer_types;
	vka_buffer_t contour_node_buffer_types;

	// Per-arc info buffers:
	vka_allocation_t arc_allocation;
	vka_buffer_t join_arc_buffer; // Indices in node position/scalar storage buffer.
	vka_buffer_t split_arc_buffer;
	vka_buffer_t contour_arc_buffer;

	// Object mesh buffers:
	vka_allocation_t mesh_allocation;
	vka_buffer_t mesh_buffer_index;
	vka_buffer_t mesh_buffer_normals;

	int (*scalar_function)(ct_tree_t *tree, ct_mesh_t *mesh, char error[NM_MAX_ERROR_LENGTH]);
	ct_mesh_t mesh;
	ct_tree_t join_tree;
	ct_tree_t split_tree;
	ct_tree_t contour_tree;

	int tree_display;
	float translate_speed;
	float rotate_speed;
	vec3 mesh_centre; // Actually negative mesh centre.

	char error[NM_MAX_ERROR_LENGTH];
} ct_program_t;

int ct_program_setup(ct_program_t *program);
void ct_program_shutdown(ct_program_t *program);
int ct_program_configure(ct_program_t *program);
int ct_program_setup_tree_meshes(ct_program_t *program);

int ct_program_object_setup(ct_program_t *program);
void ct_program_object_shutdown(ct_program_t *program);
int ct_program_prepare_mesh(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh);
int ct_program_create_object_buffers(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh);
int ct_program_create_object_allocations(ct_program_t *program);
int ct_program_upload_object_data(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh);
int ct_program_upload_helper(ct_program_t *program, vka_allocation_t *staging_allocation,
		vka_buffer_t *staging_buffer, vka_buffer_t *destination, uint8_t *data);

void ct_program_process_input(ct_program_t *program, SDL_Event *event);
void ct_program_poll_movement_keys(ct_program_t *program);
void ct_program_update_window_size(ct_program_t *program);
int ct_program_start_frame(ct_program_t *program);
int ct_program_render(ct_program_t *program);

void get_time(struct timespec *timer);
void print_time_end(FILE *file, struct timespec *start, char *message);
#endif
