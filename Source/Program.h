#ifndef CT_PROGRAM_H
#define CT_PROGRAM_H

#include <string.h>

#include <NM-Config/Config.h>
#include <Vulkan-Abstraction/Vulkan-Abstraction.h>
#include <CGLM/cglm.h>

#include "Contour-Tree.h"
#include "Mesh.h"
#include "Mesh-Loader.h"

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

	float translate_speed;
	float rotate_speed;
	vec3 mesh_centre; // Actually negative mesh centre.
	uint8_t update_scene_uniform;

	ct_scene_uniform_t scene_uniform;
	vka_allocation_t scene_uniform_allocation;
	vka_buffer_t scene_uniform_buffer;
	vka_descriptor_set_t scene_uniform_descriptor_set;

	vka_render_info_t render_info;
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

void ct_program_process_input(ct_program_t *program);
void ct_program_update_window_size(ct_program_t *program);
int ct_program_start_frame(ct_program_t *program);
int ct_program_render(ct_program_t *program);

#endif
