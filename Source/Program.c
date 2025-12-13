#include "Program.h"

int ct_program_setup(ct_program_t *program)
{
	#ifdef CT_DEBUG
	if (sizeof(ct_scene_uniform_t) > 65536)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH, "Scene uniform is too large.");
		return -1;
	}
	if ((sizeof(ct_scene_uniform_t) % 4) != 0)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Scene uniform size is not a multiple of 4.");
		return -1;
	}
	#endif

	// Vulkan base:
	strcpy(program->vulkan.name, "Contour Tree");
	program->vulkan.window_resizable = 1;
	program->vulkan.minimum_window_width = 1080;
	program->vulkan.minimum_window_height = 720;
	if (vka_setup_vulkan(&(program->vulkan)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Command buffer:
	strcpy(program->command_buffer.name, "CT command buffer");
	program->command_buffer.queue = &(program->vulkan.graphics_queue);
	if (vka_create_command_buffer(&(program->vulkan), &(program->command_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Scene uniform:
	program->update_scene_uniform = 1;
	glm_mat4_identity(program->scene_uniform.model);
	glm_mat4_identity(program->scene_uniform.view);

	strcpy(program->scene_uniform_allocation.name, "CT scene uniform allocation");
	program->scene_uniform_allocation.properties[0] = VKA_MEMORY_DEVICE;

	strcpy(program->scene_uniform_buffer.name, "CT scene uniform buffer");
	program->scene_uniform_buffer.allocation = &(program->scene_uniform_allocation);
	program->scene_uniform_buffer.data = &(program->scene_uniform);
	program->scene_uniform_buffer.usage = VKA_BUFFER_USAGE_UNIFORM;
	program->scene_uniform_buffer.size = sizeof(ct_scene_uniform_t);
	if (vka_create_buffer(&(program->vulkan), &(program->scene_uniform_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->scene_uniform_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_create_allocation(&(program->vulkan), &(program->scene_uniform_allocation)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->scene_uniform_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Descriptor pool and sets:
	strcpy(program->scene_uniform_descriptor_set.name, "CT scene uniform descriptor set");
	program->scene_uniform_descriptor_set.data = malloc(1 * sizeof(void *));
	if (!program->scene_uniform_descriptor_set.data)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for scene uniform descriptor data.");
		return -1;
	}
	program->scene_uniform_descriptor_set.data[0] = &(program->scene_uniform_buffer);
	program->scene_uniform_descriptor_set.pool = &(program->descriptor_pool);
	program->scene_uniform_descriptor_set.binding		= 0;
	program->scene_uniform_descriptor_set.count		= 1;
	program->scene_uniform_descriptor_set.type		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	program->scene_uniform_descriptor_set.stage_flags	= VK_SHADER_STAGE_ALL_GRAPHICS;
	if (vka_create_descriptor_set_layout(&(program->vulkan),
		&(program->scene_uniform_descriptor_set)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	strcpy(program->descriptor_pool.name, "CT descriptor pool");
	if (vka_create_descriptor_pool(&(program->vulkan), &(program->descriptor_pool)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	if (vka_allocate_descriptor_set(&(program->vulkan),
		&(program->scene_uniform_descriptor_set)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	if (vka_update_descriptor_set(&(program->vulkan), &(program->scene_uniform_descriptor_set)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Pipelines:
	strcpy(program->mesh_pipeline.name, "CT mesh pipeline");
	strcpy(program->mesh_pipeline.shaders[VKA_SHADER_TYPE_VERTEX].path,
						"Shaders/Mesh.vert.sprv");
	strcpy(program->mesh_pipeline.shaders[VKA_SHADER_TYPE_FRAGMENT].path,
						"Shaders/Mesh.frag.sprv");
	program->mesh_pipeline.num_descriptor_sets = 1;
	program->mesh_pipeline.descriptor_sets = malloc(program->mesh_pipeline.num_descriptor_sets *
								sizeof(vka_descriptor_set_t *));
	if (!program->mesh_pipeline.descriptor_sets)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for mesh pipeline descriptor sets.");
		return -1;
	}
	program->mesh_pipeline.descriptor_sets[0] = &(program->scene_uniform_descriptor_set);
	program->mesh_pipeline.num_vertex_bindings = 3;
	program->mesh_pipeline.strides[0] = sizeof(ct_vertex_t);
	program->mesh_pipeline.strides[1] = sizeof(ct_normal_t);
	program->mesh_pipeline.strides[2] = sizeof(float); // Scalar value.
	program->mesh_pipeline.num_vertex_attributes = 3;
	program->mesh_pipeline.bindings[0] = 0;
	program->mesh_pipeline.bindings[1] = 1;
	program->mesh_pipeline.bindings[2] = 2;
	program->mesh_pipeline.formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
	program->mesh_pipeline.formats[1] = VK_FORMAT_R8G8B8_SNORM;
	program->mesh_pipeline.formats[2] = VK_FORMAT_R32_SFLOAT;
	program->mesh_pipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	program->mesh_pipeline.colour_attachment_format = program->vulkan.swapchain_format;
	program->mesh_pipeline.blend_enable = VK_TRUE;
	// TODO depth information.

	if (vka_create_pipeline(&(program->vulkan), &(program->mesh_pipeline)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->render_info.colour_clear_value.color.float32[0] = 0.3f;
	program->render_info.colour_clear_value.color.float32[1] = 0.3f;
	program->render_info.colour_clear_value.color.float32[2] = 0.3f;
	program->render_info.colour_clear_value.color.float32[3] = 1.f;
	program->render_info.colour_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// TODO depth information. Clear value 1.f.

	ct_program_update_window_size(program);

	return 0;
}

void ct_program_shutdown(ct_program_t *program)
{
	ct_program_object_shutdown(program); // Also calls device_wait_idle.

	vka_destroy_buffer(&(program->vulkan), &(program->scene_uniform_buffer));
	vka_destroy_allocation(&(program->vulkan), &(program->scene_uniform_allocation));

	vka_destroy_pipeline(&(program->vulkan), &(program->mesh_pipeline));

	vka_destroy_descriptor_set(&(program->vulkan), &(program->scene_uniform_descriptor_set));

	vka_destroy_descriptor_pool(&(program->vulkan), &(program->descriptor_pool));
	vka_destroy_command_buffer(&(program->vulkan), &(program->command_buffer));
	vka_shutdown_vulkan(&(program->vulkan));
}

int ct_program_object_setup(ct_program_t *program)
{
	ct_mesh_gpu_ready_t gpu_mesh = {0};
	float *scalars = NULL;
	uint32_t *vertex_to_node = NULL;
	vka_allocation_t staging_allocation = {0};
	vka_buffer_t staging_buffer = {0};

	// Destroy all objects first (file dialogue will be possible in the future):
	ct_program_object_shutdown(program);

	// Load mesh:
	//if (!program->scalar_function) { program->scalar_function = ct_tree_scalar_function_y; }
	if (!program->scalar_function) { program->scalar_function = ct_tree_scalar_function_z; }
	if (ct_mesh_load(&(program->mesh), program->error)) { goto error; }

	#ifdef CT_DEBUG
	ct_mesh_print_short(stdout, &(program->mesh));
	#endif

	if (!program->mesh.is_manifold)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH, "Mesh \"%s\" is not manifold.",
									program->mesh.name);
		goto error;
	}

	// Get scalar values:
	if (program->scalar_function(&(program->join_tree), &(program->mesh), program->error))
	{
		goto error;
	}
	if (ct_tree_copy_nodes(&(program->join_tree), &(program->split_tree), program->error))
	{
		goto error;
	}

	// Construct join and split trees:
	if (ct_merge_tree_construct(&(program->join_tree), &(program->mesh),
			program->join_tree.num_nodes - 1, program->error))
	{
		goto error;
	}
	if (ct_merge_tree_construct(&(program->split_tree), &(program->mesh),
							0, program->error))
	{
		goto error;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n*************\n");
	fprintf(stdout, "* Join tree *\n");
	fprintf(stdout, "*************\n");
	ct_tree_print_short(stdout, &(program->join_tree));

	fprintf(stdout, "\n**************\n");
	fprintf(stdout, "* Split tree *\n");
	fprintf(stdout, "**************\n");
	ct_tree_print_short(stdout, &(program->split_tree));

	fprintf(stdout, "\n");
	#endif

	// Construct contour tree:
	if (ct_contour_tree_construct(&(program->contour_tree), &(program->join_tree),
					&(program->split_tree), program->error))
	{
		goto error;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n****************\n");
	fprintf(stdout, "* Contour tree *\n");
	fprintf(stdout, "****************\n");
	ct_tree_print_short(stdout, &(program->contour_tree));
	#endif

	// Create temporary GPU-ready mesh and reorganise scalar values:
	strcpy(gpu_mesh.name, program->mesh.name);
	if (ct_mesh_prepare_for_gpu(&(program->mesh), &gpu_mesh, program->error)) { goto error; }

	#ifdef CT_DEBUG
	fprintf(stdout, "\n");
	ct_mesh_gpu_ready_print_short(stdout, &gpu_mesh);
	#endif

	scalars = malloc(gpu_mesh.num_vertices * sizeof(float));
	vertex_to_node = malloc(program->join_tree.num_nodes * sizeof(uint32_t));
	if (!scalars || !vertex_to_node)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for GPU mesh \"%s\" scalar values.",
			gpu_mesh.name);
		goto error;
	}
	for (uint32_t i = 0; i < program->join_tree.num_nodes; i++)
	{
		vertex_to_node[program->join_tree.nodes[i].vertex] = i;
	}
	for (uint32_t i = 0; i < gpu_mesh.num_vertices; i++)
	{
		scalars[i] = program->join_tree.nodes[vertex_to_node[
				gpu_mesh.original_index[i]]].value;
	}

	program->scene_uniform.max_value = program->join_tree.nodes[
			program->join_tree.num_nodes - 1].value;
	program->scene_uniform.min_value = program->join_tree.nodes[0].value;
	program->scene_uniform.isovalue = program->scene_uniform.max_value;
	program->scene_uniform.highlight_size = (program->scene_uniform.max_value -
					program->scene_uniform.min_value) * 0.015f;

	// Get mesh sizes for better camera positioning:
	float limits[6];
	limits[0] = limits[1] = gpu_mesh.vertices[0].x;
	limits[2] = limits[3] = gpu_mesh.vertices[0].y;
	limits[4] = limits[5] = gpu_mesh.vertices[0].z;
	for (uint32_t i = 0; i < gpu_mesh.num_vertices; i++)
	{
		if (gpu_mesh.vertices[i].x < limits[0]) { limits[0] = gpu_mesh.vertices[i].x; }
		if (gpu_mesh.vertices[i].x > limits[1]) { limits[1] = gpu_mesh.vertices[i].x; }
		if (gpu_mesh.vertices[i].y < limits[2]) { limits[2] = gpu_mesh.vertices[i].y; }
		if (gpu_mesh.vertices[i].y > limits[3]) { limits[3] = gpu_mesh.vertices[i].y; }
		if (gpu_mesh.vertices[i].z < limits[4]) { limits[4] = gpu_mesh.vertices[i].z; }
		if (gpu_mesh.vertices[i].z > limits[5]) { limits[5] = gpu_mesh.vertices[i].z; }
	}

	program->mesh_centre[0] = -(limits[0] + limits[1]) / 2.f;
	program->mesh_centre[1] = -(limits[2] + limits[3]) / 2.f;
	program->mesh_centre[2] = -(limits[4] + limits[5]) / 2.f;

	program->rotate_speed = 0.025f;
	program->translate_speed = (limits[0] - limits[1]) / 2.f;
	if (program->translate_speed < 0.f) { program->translate_speed *= -1.f; }
	if (program->translate_speed == 0.f) { program->translate_speed = 0.01f; }

	glm_translate_z(program->scene_uniform.view, -(2.5f * limits[5]));
	glm_translate(program->scene_uniform.model, program->mesh_centre);

	// Create mesh allocation and index/vertex buffers:
	strcpy(program->buffer_allocation.name, "CT mesh allocation");
	program->buffer_allocation.properties[0] = VKA_MEMORY_DEVICE;
	program->index_buffer.index_type = VK_INDEX_TYPE_UINT32;

	vka_buffer_t *buffers[4] = { &(program->index_buffer),
				&(program->vertex_buffers[0]),
				&(program->vertex_buffers[1]),
				&(program->vertex_buffers[2]) };
	VkBufferUsageFlags usage_flags[4] = { VKA_BUFFER_USAGE_INDEX, VKA_BUFFER_USAGE_VERTEX,
					VKA_BUFFER_USAGE_VERTEX, VKA_BUFFER_USAGE_VERTEX };
	char *names[4] = { "Mesh indices", "Mesh positions", "Mesh normals", "Mesh scalars" };
	VkDeviceSize sizes[4] = { gpu_mesh.num_faces * sizeof(ct_face_gpu_ready_t),
				gpu_mesh.num_vertices * sizeof(ct_vertex_t),
				gpu_mesh.num_vertices * sizeof(ct_normal_t),
				gpu_mesh.num_vertices * sizeof(float) };

	for (int i = 0; i < 4; i++)
	{
		strcpy(buffers[i]->name, names[i]);
		buffers[i]->allocation = &(program->buffer_allocation);
		buffers[i]->size = sizes[i];
		buffers[i]->usage = usage_flags[i];

		if (vka_create_buffer(&(program->vulkan), buffers[i])) { goto error; }
		if (vka_get_buffer_requirements(&(program->vulkan), buffers[i])) { goto error; }
	}

	if (vka_create_allocation(&(program->vulkan), &(program->buffer_allocation)))
	{
		goto error;
	}

	for (int i = 0; i < 4; i++)
	{
		if (vka_bind_buffer_memory(&(program->vulkan), buffers[i])) { goto error; }
	}

	// Create staging buffer for GPU upload (largest size will be needed for mesh data):
	strcpy(staging_allocation.name, "Staging allocation");
	staging_allocation.properties[0] = VKA_MEMORY_HOST;

	strcpy(staging_buffer.name, "Staging buffer");
	staging_buffer.allocation = &staging_allocation;
	staging_buffer.usage = VKA_BUFFER_USAGE_STAGING;
	staging_buffer.size = sizes[0];
	for (int i = 1; i < 4; i++)
	{
		if (sizes[i] > staging_buffer.size) { staging_buffer.size = sizes[i]; }
	}

	if (vka_create_buffer(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_get_buffer_requirements(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_create_allocation(&(program->vulkan), &staging_allocation)) { goto error; }
	if (vka_bind_buffer_memory(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_map_memory(&(program->vulkan), &staging_allocation)) { goto error; }

	// Upload mesh data to buffers:
	uint8_t *data[4] = { (uint8_t *)(gpu_mesh.faces), (uint8_t *)(gpu_mesh.vertices),
			(uint8_t *)(gpu_mesh.normals), (uint8_t *)(scalars) };

	for (int i = 0; i < 4; i++)
	{
		if (vka_begin_command_buffer(&(program->vulkan), &(program->command_buffer)))
		{
			goto error;
		}

		memcpy(staging_allocation.mapped_data, data[i], sizes[i]);
		vka_copy_buffer(&(program->command_buffer), &staging_buffer, buffers[i]);

		if ((vka_end_command_buffer(&(program->vulkan), &(program->command_buffer))) ||
		(vka_submit_command_buffer(&(program->vulkan), &(program->command_buffer))) ||
		(vka_wait_for_fence(&(program->vulkan), &(program->command_buffer))))
		{
			goto error;
		}
	}



	// Upload tree data to buffers:

	// Cleanup, success:
	vka_destroy_buffer(&(program->vulkan), &staging_buffer);
	vka_destroy_allocation(&(program->vulkan), &staging_allocation);
	free(vertex_to_node);
	free(scalars);
	ct_mesh_gpu_ready_free(&gpu_mesh);
	return 0;

	// Cleanup, failure:
	error:
	if (strcmp(program->vulkan.error, "")) { strcpy(program->error, program->vulkan.error); }
	vka_destroy_buffer(&(program->vulkan), &staging_buffer);
	vka_destroy_allocation(&(program->vulkan), &staging_allocation);
	if (vertex_to_node) { free(vertex_to_node); }
	if (scalars) { free(scalars); }
	ct_mesh_gpu_ready_free(&gpu_mesh);
	return -1;
}

void ct_program_object_shutdown(ct_program_t *program)
{
	vka_device_wait_idle(&(program->vulkan));

	for (int i = 0; i < 3; i++)
	{
		vka_destroy_buffer(&(program->vulkan), &(program->vertex_buffers[i]));
	}
	vka_destroy_buffer(&(program->vulkan), &(program->index_buffer));
	vka_destroy_allocation(&(program->vulkan), &(program->buffer_allocation));

	ct_tree_free(&(program->contour_tree));
	ct_tree_free(&(program->split_tree));
	ct_tree_free(&(program->join_tree));
	ct_mesh_free(&(program->mesh));
}

void ct_program_process_input(ct_program_t *program)
{
	const bool *keyboard_state = SDL_GetKeyboardState(NULL);

	int changed = 0;
	static float translate_z = 0.f;
	static float rotate_y = 0.f;
	vec3 axis = { 0.f, 1.f, 0.f };

	if (keyboard_state[SDL_SCANCODE_SPACE])
	{
		translate_z = 0.f;
		rotate_y = 0.f;
		changed = 1;
	}
	else if (keyboard_state[SDL_SCANCODE_RIGHT])
	{
		rotate_y += program->rotate_speed;
		while (rotate_y > 360.f) { rotate_y -= 360.f; }
		changed = 1;
	}
	else if (keyboard_state[SDL_SCANCODE_LEFT])
	{
		rotate_y -= program->rotate_speed;
		while (rotate_y < -360.f) { rotate_y += 360.f; }
		changed = 1;
	}
	else if (keyboard_state[SDL_SCANCODE_UP])
	{
		translate_z += program->translate_speed;
		if (translate_z > 10000.f) { translate_z = 10000.f; }
		changed = 1;
	}
	else if (keyboard_state[SDL_SCANCODE_DOWN])
	{
		translate_z -= program->translate_speed;
		if (translate_z < -10000.f) { translate_z = -10000.f; }
		changed = 1;
	}

	if (changed)
	{
		glm_mat4_identity(program->scene_uniform.model);
		glm_translate_z(program->scene_uniform.model, translate_z);
		glm_rotate(program->scene_uniform.model, rotate_y, axis);
		glm_translate(program->scene_uniform.model, program->mesh_centre);
		program->update_scene_uniform = 1;
	}
}

void ct_program_update_window_size(ct_program_t *program)
{
	// Update render info with new swapchain extent:
	program->render_info.render_area.extent.width = program->vulkan.swapchain_extent.width;
	program->render_info.render_area.extent.height = program->vulkan.swapchain_extent.height;
	program->render_info.render_target_height = program->vulkan.swapchain_extent.height;
	program->render_info.scissor_area.extent.width = program->vulkan.swapchain_extent.width;
	program->render_info.scissor_area.extent.height = program->vulkan.swapchain_extent.height;

	// Update projection matrix:
	glm_perspective(1.f, (program->vulkan.swapchain_extent.width * 1.f) /
		(program->vulkan.swapchain_extent.height * 1.f), CT_CLIP_NEAR, CT_CLIP_FAR,
		program->scene_uniform.projection);
	program->update_scene_uniform = 1;
}

int ct_program_start_frame(ct_program_t *program)
{
	if (vka_get_next_swapchain_image(&(program->vulkan)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (program->vulkan.recreate_swapchain)
	{
		if (vka_create_swapchain(&(program->vulkan)))
		{
			strcpy(program->error, program->vulkan.error);
			return -1;
		}
		program->vulkan.recreate_swapchain = 0;
		ct_program_update_window_size(program);
		if (vka_get_next_swapchain_image(&(program->vulkan)))
		{
			strcpy(program->error, program->vulkan.error);
			return -1;
		}
	}
	if (program->vulkan.recreate_pipelines)
	{
		program->mesh_pipeline.colour_attachment_format = program->vulkan.swapchain_format;

		if (vka_create_pipeline(&(program->vulkan), &(program->mesh_pipeline)))
		{
			strcpy(program->error, program->vulkan.error);
			return -1;
		}

		program->vulkan.recreate_pipelines = 0;
	}

	return 0;
}

int ct_program_render(ct_program_t *program)
{
	/* Render contour tree first, depth write ON, depth read ON.
	 * Render mesh next, depth write OFF, depth read ON.
	 * Render program last, depth write OFF, depth read OFF. */

	if (vka_begin_command_buffer(&(program->vulkan),
		&(program->vulkan.command_buffers[program->vulkan.current_frame])))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	if (program->update_scene_uniform)
	{
		vka_update_buffer(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->scene_uniform_buffer));
		program->update_scene_uniform = 0;
	}

	program->render_info.colour_image = &(program->vulkan.swapchain_images[
				program->vulkan.current_swapchain_index]);
	program->render_info.colour_image_view = &(program->vulkan.swapchain_image_views[
					program->vulkan.current_swapchain_index]);

	vka_begin_rendering(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->render_info));

	vka_set_viewport(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->render_info));
	vka_set_scissor(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->render_info));

	vka_bind_pipeline(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->mesh_pipeline));

	vka_bind_descriptor_sets(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
									&(program->mesh_pipeline));

	vka_bind_vertex_buffers(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
					&(program->index_buffer), 3, program->vertex_buffers);

	vka_draw_indexed(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
							program->mesh.num_faces * 3, 0, 0);

	vka_end_rendering(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->render_info));

	if (vka_end_command_buffer(&(program->vulkan),
		&(program->vulkan.command_buffers[program->vulkan.current_frame])))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	if (vka_submit_command_buffer(&(program->vulkan),
		&(program->vulkan.command_buffers[program->vulkan.current_frame])) != VK_SUCCESS)
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	if (vka_present_image(&(program->vulkan)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	vka_next_frame(&(program->vulkan));

	return 0;
}
