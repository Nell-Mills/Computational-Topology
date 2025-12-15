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

	// Set config for rest of program:
	if (ct_program_configure(program)) { return -1; }

	// Command buffer:
	if (vka_create_command_buffer(&(program->vulkan), &(program->command_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Scene uniform:
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
	if (vka_create_descriptor_set_layout(&(program->vulkan),
		&(program->scene_uniform_descriptor_set)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

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
	if (vka_create_pipeline(&(program->vulkan), &(program->node_pipeline)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_create_pipeline(&(program->vulkan), &(program->arc_pipeline)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_create_pipeline(&(program->vulkan), &(program->mesh_pipeline)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	ct_program_update_window_size(program);

	return 0;
}

void ct_program_shutdown(ct_program_t *program)
{
	ct_program_object_shutdown(program); // Also calls device_wait_idle.

	vka_destroy_descriptor_set(&(program->vulkan), &(program->node_descriptor_set));

	vka_destroy_buffer(&(program->vulkan), &(program->cylinder_buffer_normals));
	vka_destroy_buffer(&(program->vulkan), &(program->cylinder_buffer_positions));
	vka_destroy_buffer(&(program->vulkan), &(program->cylinder_buffer_index));
	vka_destroy_buffer(&(program->vulkan), &(program->sphere_buffer_normals));
	vka_destroy_buffer(&(program->vulkan), &(program->sphere_buffer_positions));
	vka_destroy_buffer(&(program->vulkan), &(program->sphere_buffer_index));
	vka_destroy_allocation(&(program->vulkan), &(program->tree_mesh_allocation));

	vka_destroy_buffer(&(program->vulkan), &(program->scene_uniform_buffer));
	vka_destroy_allocation(&(program->vulkan), &(program->scene_uniform_allocation));
	vka_destroy_descriptor_set(&(program->vulkan), &(program->scene_uniform_descriptor_set));

	vka_destroy_pipeline(&(program->vulkan), &(program->mesh_pipeline));
	vka_destroy_pipeline(&(program->vulkan), &(program->arc_pipeline));
	vka_destroy_pipeline(&(program->vulkan), &(program->node_pipeline));

	vka_destroy_descriptor_pool(&(program->vulkan), &(program->descriptor_pool));
	vka_destroy_command_buffer(&(program->vulkan), &(program->command_buffer));
	vka_shutdown_vulkan(&(program->vulkan));
}

int ct_program_configure(ct_program_t *program)
{
	// Command buffer:
	strcpy(program->command_buffer.name, "CT command buffer");
	program->command_buffer.queue = &(program->vulkan.graphics_queue);

	// Descriptor pool:
	strcpy(program->descriptor_pool.name, "CT descriptor pool");

	// Rendering info:
	program->render_info.colour_clear_value.color.float32[0] = 0.3f;
	program->render_info.colour_clear_value.color.float32[1] = 0.3f;
	program->render_info.colour_clear_value.color.float32[2] = 0.3f;
	program->render_info.colour_clear_value.color.float32[3] = 1.f;
	program->render_info.colour_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// TODO depth information. Clear value 1.f.

	// Node pipeline:
	strcpy(program->node_pipeline.name, "CT node pipeline");
	strcpy(program->node_pipeline.shaders[VKA_SHADER_TYPE_VERTEX].path,
						"Shaders/Node.vert.sprv");
	strcpy(program->node_pipeline.shaders[VKA_SHADER_TYPE_FRAGMENT].path,
						"Shaders/Node.frag.sprv");
	program->node_pipeline.num_descriptor_sets = 1;
	program->node_pipeline.descriptor_sets = malloc(program->node_pipeline.num_descriptor_sets *
								sizeof(vka_descriptor_set_t *));
	if (!program->node_pipeline.descriptor_sets)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for node pipeline descriptor sets.");
		return -1;
	}
	program->node_pipeline.descriptor_sets[0] = &(program->scene_uniform_descriptor_set);
	program->node_pipeline.num_vertex_bindings = 1;
	program->node_pipeline.strides[0] = 4 * sizeof(float); // Positions/scalars.
	program->node_pipeline.num_vertex_attributes = 1;
	program->node_pipeline.bindings[0] = 0;
	program->node_pipeline.formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	program->node_pipeline.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	program->node_pipeline.cull_mode = VK_CULL_MODE_BACK_BIT;
	program->node_pipeline.blend_enable = VK_TRUE;
	program->node_pipeline.colour_attachment_format = program->vulkan.swapchain_format;
	// TODO depth information.

	// Arc pipeline:
	strcpy(program->arc_pipeline.name, "CT arc pipeline");
	strcpy(program->arc_pipeline.shaders[VKA_SHADER_TYPE_VERTEX].path,
						"Shaders/Arc.vert.sprv");
	strcpy(program->arc_pipeline.shaders[VKA_SHADER_TYPE_FRAGMENT].path,
						"Shaders/Arc.frag.sprv");
	program->arc_pipeline.num_descriptor_sets = 1;
	program->arc_pipeline.descriptor_sets = malloc(program->arc_pipeline.num_descriptor_sets *
								sizeof(vka_descriptor_set_t *));
	if (!program->arc_pipeline.descriptor_sets)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for arc pipeline descriptor sets.");
		return -1;
	}
	program->arc_pipeline.descriptor_sets[0] = &(program->scene_uniform_descriptor_set);
	program->arc_pipeline.num_vertex_bindings = 1;
	program->arc_pipeline.strides[0] = 4 * sizeof(float); // Positions/scalars.
	program->arc_pipeline.num_vertex_attributes = 1;
	program->arc_pipeline.bindings[0] = 0;
	program->arc_pipeline.formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	program->arc_pipeline.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	program->arc_pipeline.blend_enable = VK_TRUE;
	program->arc_pipeline.colour_attachment_format = program->vulkan.swapchain_format;
	// TODO depth information.

	// Mesh pipeline:
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
	program->mesh_pipeline.num_vertex_bindings = 2;
	program->mesh_pipeline.strides[0] = 4 * sizeof(float); // Positions/scalars.
	program->mesh_pipeline.strides[1] = sizeof(ct_normal_t);
	program->mesh_pipeline.num_vertex_attributes = 2;
	program->mesh_pipeline.bindings[0] = 0;
	program->mesh_pipeline.bindings[1] = 1;
	program->mesh_pipeline.formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	program->mesh_pipeline.formats[1] = VK_FORMAT_R8G8B8_SNORM;
	program->mesh_pipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	program->mesh_pipeline.cull_mode = VK_CULL_MODE_BACK_BIT;
	program->mesh_pipeline.blend_enable = VK_TRUE;
	program->mesh_pipeline.colour_attachment_format = program->vulkan.swapchain_format;
	// TODO depth information.

	// Scene uniform:
	program->update_scene_uniform = 1;
	glm_mat4_identity(program->scene_uniform.model);
	glm_mat4_identity(program->scene_uniform.view);

	// Scene uniform allocation:
	strcpy(program->scene_uniform_allocation.name, "CT scene uniform allocation");
	program->scene_uniform_allocation.properties[0] = VKA_MEMORY_DEVICE;

	// Scene uniform buffer:
	strcpy(program->scene_uniform_buffer.name, "CT scene uniform buffer");
	program->scene_uniform_buffer.allocation = &(program->scene_uniform_allocation);
	program->scene_uniform_buffer.data = &(program->scene_uniform);
	program->scene_uniform_buffer.usage = VKA_BUFFER_USAGE_UNIFORM;
	program->scene_uniform_buffer.size = sizeof(ct_scene_uniform_t);

	// Scene uniform descriptor set:
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

	// Tree mesh data:
	strcpy(program->tree_mesh_allocation.name, "CT tree mesh allocation");
	program->tree_mesh_allocation.properties[0] = VKA_MEMORY_DEVICE;

	strcpy(program->sphere_buffer_index.name, "CT sphere index buffer");
	program->sphere_buffer_index.allocation = &(program->tree_mesh_allocation);
	program->sphere_buffer_index.usage = VKA_BUFFER_USAGE_INDEX;
	program->sphere_buffer_index.index_type = VK_INDEX_TYPE_UINT32;

	strcpy(program->sphere_buffer_positions.name, "CT sphere position buffer");
	program->sphere_buffer_positions.allocation = &(program->tree_mesh_allocation);
	program->sphere_buffer_positions.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->sphere_buffer_normals.name, "CT sphere normal buffer");
	program->sphere_buffer_normals.allocation = &(program->tree_mesh_allocation);
	program->sphere_buffer_normals.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->cylinder_buffer_index.name, "CT cylinder index buffer");
	program->cylinder_buffer_index.allocation = &(program->tree_mesh_allocation);
	program->cylinder_buffer_index.usage = VKA_BUFFER_USAGE_INDEX;
	program->cylinder_buffer_index.index_type = VK_INDEX_TYPE_UINT32;

	strcpy(program->cylinder_buffer_positions.name, "CT cylinder position buffer");
	program->cylinder_buffer_positions.allocation = &(program->tree_mesh_allocation);
	program->cylinder_buffer_positions.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->cylinder_buffer_normals.name, "CT cylinder normal buffer");
	program->cylinder_buffer_normals.allocation = &(program->tree_mesh_allocation);
	program->cylinder_buffer_normals.usage = VKA_BUFFER_USAGE_VERTEX;

	// Node data:
	strcpy(program->node_allocation.name, "CT node allocation");
	program->node_allocation.properties[0] = VKA_MEMORY_DEVICE;

	strcpy(program->node_buffer_positions_scalars.name, "CT node position/scalar buffer");
	program->node_buffer_positions_scalars.allocation = &(program->node_allocation);
	program->node_buffer_positions_scalars.usage = VKA_BUFFER_USAGE_VERTEX |
						VKA_BUFFER_USAGE_STORAGE;

	strcpy(program->join_node_buffer_types.name, "CT join node type buffer");
	program->join_node_buffer_types.allocation = &(program->node_allocation);
	program->join_node_buffer_types.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->split_node_buffer_types.name, "CT split node type buffer");
	program->split_node_buffer_types.allocation = &(program->node_allocation);
	program->split_node_buffer_types.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->contour_node_buffer_types.name, "CT contour node type buffer");
	program->contour_node_buffer_types.allocation = &(program->node_allocation);
	program->contour_node_buffer_types.usage = VKA_BUFFER_USAGE_VERTEX;

	// Arc data:
	strcpy(program->arc_allocation.name, "CT arc allocation");
	program->arc_allocation.properties[0] = VKA_MEMORY_DEVICE;

	strcpy(program->join_arc_buffer.name, "CT join tree arc endpoint buffer");
	program->join_arc_buffer.allocation = &(program->arc_allocation);
	program->join_arc_buffer.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->split_arc_buffer.name, "CT split tree arc endpoint buffer");
	program->split_arc_buffer.allocation = &(program->arc_allocation);
	program->split_arc_buffer.usage = VKA_BUFFER_USAGE_VERTEX;

	strcpy(program->contour_arc_buffer.name, "CT contour tree arc endpoint buffer");
	program->contour_arc_buffer.allocation = &(program->arc_allocation);
	//program->contour_arc_buffer.usage = VKA_BUFFER_USAGE_VERTEX;
	program->contour_arc_buffer.usage = VKA_BUFFER_USAGE_INDEX;
	program->contour_arc_buffer.index_type = VK_INDEX_TYPE_UINT32;

	// Object mesh data:
	strcpy(program->mesh_allocation.name, "CT mesh allocation");
	program->mesh_allocation.properties[0] = VKA_MEMORY_DEVICE;

	strcpy(program->mesh_buffer_index.name, "CT mesh index buffer");
	program->mesh_buffer_index.allocation = &(program->mesh_allocation);
	program->mesh_buffer_index.usage = VKA_BUFFER_USAGE_INDEX;
	program->mesh_buffer_index.index_type = VK_INDEX_TYPE_UINT32;

	strcpy(program->mesh_buffer_normals.name, "CT mesh normal buffer");
	program->mesh_buffer_normals.allocation = &(program->mesh_allocation);
	program->mesh_buffer_normals.usage = VKA_BUFFER_USAGE_VERTEX;

	return 0;
}

int ct_program_object_setup(ct_program_t *program)
{
	// Destroy all objects first (file dialogue will be possible in the future):
	ct_program_object_shutdown(program);

	// Load mesh:
	if (ct_mesh_load(&(program->mesh), program->error)) { return -1; }

	#ifdef CT_DEBUG
	ct_mesh_print_short(stdout, &(program->mesh));
	#endif

	if (!program->mesh.is_manifold)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH, "Mesh \"%s\" is not manifold.",
									program->mesh.name);
		return -1;
	}

	// Get scalar values:
	if (!program->scalar_function) { program->scalar_function = ct_tree_scalar_function_y; }
	if (program->scalar_function(&(program->join_tree), &(program->mesh), program->error))
	{
		return -1;
	}
	if (ct_tree_copy_nodes(&(program->join_tree), &(program->split_tree), program->error))
	{
		return -1;
	}

	// Construct join and split trees:
	if (ct_merge_tree_construct(&(program->join_tree), &(program->mesh),
			program->join_tree.num_nodes - 1, program->error))
	{
		return -1;
	}
	if (ct_merge_tree_construct(&(program->split_tree), &(program->mesh),
							0, program->error))
	{
		return -1;
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
	#endif

	// Construct contour tree:
	if (ct_contour_tree_construct(&(program->contour_tree), &(program->join_tree),
					&(program->split_tree), program->error))
	{
		return -1;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n****************\n");
	fprintf(stdout, "* Contour tree *\n");
	fprintf(stdout, "****************\n");
	ct_tree_print_short(stdout, &(program->contour_tree));
	#endif

	// Get scalar limits:
	program->scene_uniform.max_value = program->join_tree.nodes[
			program->join_tree.num_nodes - 1].value;
	program->scene_uniform.min_value = program->join_tree.nodes[0].value;
	program->scene_uniform.isovalue = program->scene_uniform.max_value;
	program->scene_uniform.highlight_size = (program->scene_uniform.max_value -
					program->scene_uniform.min_value) * 0.015f;

	// Create temporary GPU-ready mesh:
	ct_mesh_gpu_ready_t gpu_mesh = {0};
	if (ct_program_prepare_mesh(program, &gpu_mesh))
	{
		return -1;
	}

	#ifdef CT_DEBUG
	fprintf(stdout, "\n");
	ct_mesh_gpu_ready_print_short(stdout, &gpu_mesh);
	#endif

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

	// Set up object buffers:
	if (ct_program_create_object_buffers(program, &gpu_mesh)) { return -1; }
	if (ct_program_create_object_allocations(program)) { return -1; }
	if (ct_program_upload_object_data(program, &gpu_mesh)) { return -1; }

	ct_mesh_gpu_ready_free(&gpu_mesh);
	return 0;
}

void ct_program_object_shutdown(ct_program_t *program)
{
	vka_device_wait_idle(&(program->vulkan));

	// Per-node info buffers:
	vka_destroy_buffer(&(program->vulkan), &(program->node_buffer_positions_scalars));
	vka_destroy_buffer(&(program->vulkan), &(program->contour_node_buffer_types));
	vka_destroy_buffer(&(program->vulkan), &(program->split_node_buffer_types));
	vka_destroy_buffer(&(program->vulkan), &(program->join_node_buffer_types));
	vka_destroy_allocation(&(program->vulkan), &(program->node_allocation));

	// Per-arc info buffers:
	vka_destroy_buffer(&(program->vulkan), &(program->contour_arc_buffer));
	vka_destroy_buffer(&(program->vulkan), &(program->split_arc_buffer));
	vka_destroy_buffer(&(program->vulkan), &(program->join_arc_buffer));
	vka_destroy_allocation(&(program->vulkan), &(program->arc_allocation));

	// Object mesh buffers:
	vka_destroy_buffer(&(program->vulkan), &(program->mesh_buffer_normals));
	vka_destroy_buffer(&(program->vulkan), &(program->mesh_buffer_index));
	vka_destroy_allocation(&(program->vulkan), &(program->mesh_allocation));

	ct_tree_free(&(program->contour_tree));
	ct_tree_free(&(program->split_tree));
	ct_tree_free(&(program->join_tree));
	ct_mesh_free(&(program->mesh));
}

int ct_program_prepare_mesh(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh)
{
	if (ct_mesh_check_validity(&(program->mesh), program->error)) { return -1; }
	if (!program->join_tree.nodes)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH, "Join tree has no nodes.");
		return -1;
	}
	if (program->mesh.num_vertices != program->join_tree.num_nodes)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH, "Number of vertices in mesh \"%s\" "
			"doesn't match number of nodes in join tree.", program->mesh.name);
		return -1;
	}

	// Only need a subset of attributes, so allocate here:
	ct_mesh_gpu_ready_free(gpu_mesh);
	strcpy(gpu_mesh->name, program->mesh.name);
	gpu_mesh->num_vertices = program->mesh.num_vertices;
	gpu_mesh->num_faces = program->mesh.num_faces;
	gpu_mesh->vertices = malloc(gpu_mesh->num_vertices * sizeof(ct_vertex_t));
	gpu_mesh->normals = malloc(gpu_mesh->num_vertices * sizeof(ct_normal_t));
	gpu_mesh->faces = malloc(gpu_mesh->num_faces * sizeof(ct_face_gpu_ready_t));
	if (!gpu_mesh->vertices || !gpu_mesh->normals || !gpu_mesh->faces)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for GPU-ready mesh \"%s\".", gpu_mesh->name);
		ct_mesh_gpu_ready_free(gpu_mesh);
		return -1;
	}

	// Reorder vertices to match nodes (so that the mesh and trees can share the buffer):
	for (uint32_t i = 0; i < gpu_mesh->num_vertices; i++)
	{
		memcpy(&(gpu_mesh->vertices[i]), &(program->mesh.vertices[
			program->join_tree.nodes[i].node_to_vertex]), sizeof(ct_vertex_t));
	}
	for (uint32_t i = 0; i < gpu_mesh->num_faces; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			gpu_mesh->faces[i][j] = program->join_tree.nodes[
				program->mesh.faces[i][j].v].vertex_to_node;
		}
	}

	// Get per-vertex normals:
	if (ct_mesh_gpu_ready_vertex_normals(gpu_mesh, program->error)) { return -1; }

	return 0;
}

int ct_program_create_object_buffers(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh)
{
	// Per-node info buffers:
	program->node_buffer_positions_scalars.size = program->join_tree.num_nodes *
								4 * sizeof(float);
	if (vka_create_buffer(&(program->vulkan), &(program->node_buffer_positions_scalars)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan),
		&(program->node_buffer_positions_scalars)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->join_node_buffer_types.size = program->join_tree.num_nodes * sizeof(int8_t);
	if (vka_create_buffer(&(program->vulkan), &(program->join_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->join_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->split_node_buffer_types.size = program->split_tree.num_nodes * sizeof(int8_t);
	if (vka_create_buffer(&(program->vulkan), &(program->split_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->split_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->contour_node_buffer_types.size = program->contour_tree.num_nodes * sizeof(int8_t);
	if (vka_create_buffer(&(program->vulkan), &(program->contour_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->contour_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Per-arc info buffers:
	program->join_arc_buffer.size = program->join_tree.num_arcs * 2 * sizeof(uint32_t);
	if (vka_create_buffer(&(program->vulkan), &(program->join_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->join_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->split_arc_buffer.size = program->split_tree.num_arcs * 2 * sizeof(uint32_t);
	if (vka_create_buffer(&(program->vulkan), &(program->split_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->split_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->contour_arc_buffer.size = program->contour_tree.num_arcs * 2 * sizeof(uint32_t);
	if (vka_create_buffer(&(program->vulkan), &(program->contour_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->contour_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Object mesh buffers:
	program->mesh_buffer_index.size = gpu_mesh->num_faces * sizeof(ct_face_gpu_ready_t);
	if (vka_create_buffer(&(program->vulkan), &(program->mesh_buffer_index)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->mesh_buffer_index)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	program->mesh_buffer_normals.size = gpu_mesh->num_vertices * sizeof(ct_normal_t);
	if (vka_create_buffer(&(program->vulkan), &(program->mesh_buffer_normals)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_get_buffer_requirements(&(program->vulkan), &(program->mesh_buffer_normals)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	return 0;
}

int ct_program_create_object_allocations(ct_program_t *program)
{
	// Per-node info buffers:
	if (vka_create_allocation(&(program->vulkan), &(program->node_allocation)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->node_buffer_positions_scalars)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->join_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->split_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->contour_node_buffer_types)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Per-arc info buffers:
	if (vka_create_allocation(&(program->vulkan), &(program->arc_allocation)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->join_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->split_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->contour_arc_buffer)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	// Object mesh buffers:
	if (vka_create_allocation(&(program->vulkan), &(program->mesh_allocation)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->mesh_buffer_index)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}
	if (vka_bind_buffer_memory(&(program->vulkan), &(program->mesh_buffer_normals)))
	{
		strcpy(program->error, program->vulkan.error);
		return -1;
	}

	return 0;
}

int ct_program_upload_object_data(ct_program_t *program, ct_mesh_gpu_ready_t *gpu_mesh)
{
	vka_allocation_t staging_allocation = {0};
	vka_buffer_t staging_buffer = {0};
	float *positions_scalars = NULL;
	int8_t *types = NULL;
	uint32_t *arc_endpoints = NULL;

	// Create staging buffer for GPU upload:
	strcpy(staging_allocation.name, "Staging allocation");
	staging_allocation.properties[0] = VKA_MEMORY_HOST;

	strcpy(staging_buffer.name, "Staging buffer");
	staging_buffer.allocation = &staging_allocation;
	staging_buffer.usage = VKA_BUFFER_USAGE_STAGING;

	// Largest buffer is either node positions/scalars, or mesh index buffer:
	staging_buffer.size = program->node_buffer_positions_scalars.size;
	if (program->mesh_buffer_index.size > staging_buffer.size)
	{
		staging_buffer.size = program->mesh_buffer_index.size;
	}

	if (vka_create_buffer(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_get_buffer_requirements(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_create_allocation(&(program->vulkan), &staging_allocation)) { goto error; }
	if (vka_bind_buffer_memory(&(program->vulkan), &staging_buffer)) { goto error; }
	if (vka_map_memory(&(program->vulkan), &staging_allocation)){ goto error; }

	// Upload positions/scalar values:
	positions_scalars = malloc(program->join_tree.num_nodes * 4 * sizeof(float));
	if (!positions_scalars)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for upload of positions/scalars.");
		goto error;
	}
	for (uint32_t i = 0; i < program->join_tree.num_nodes; i++)
	{
		positions_scalars[i * 4] = gpu_mesh->vertices[i].x;
		positions_scalars[(i * 4) + 1] = gpu_mesh->vertices[i].y;
		positions_scalars[(i * 4) + 2] = gpu_mesh->vertices[i].z;
		positions_scalars[(i * 4) + 3] = program->join_tree.nodes[i].value;
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
		&(program->node_buffer_positions_scalars), (uint8_t *)(positions_scalars)))
	{
		goto error;
	}
	free(positions_scalars);
	positions_scalars = NULL;

	// Allocate memory for node types:
	types = malloc(program->join_tree.num_nodes * sizeof(int8_t));
	if (!types)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for upload of node types.");
		goto error;
	}

	// Upload join node types:
	for (uint32_t i = 0; i < program->join_tree.num_nodes; i++)
	{
		types[i] = ct_tree_get_node_type(&(program->join_tree.nodes[i]));
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
			&(program->join_node_buffer_types), (uint8_t *)(types)))
	{
		goto error;
	}

	// Upload split node types:
	for (uint32_t i = 0; i < program->split_tree.num_nodes; i++)
	{
		types[i] = ct_tree_get_node_type(&(program->split_tree.nodes[i]));
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
			&(program->split_node_buffer_types), (uint8_t *)(types)))
	{
		goto error;
	}

	// Upload contour node types:
	for (uint32_t i = 0; i < program->contour_tree.num_nodes; i++)
	{
		types[i] = ct_tree_get_node_type(&(program->contour_tree.nodes[i]));
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
			&(program->contour_node_buffer_types), (uint8_t *)(types)))
	{
		goto error;
	}

	free(types);
	types = NULL;

	// Allocate memory for arc index data:
	arc_endpoints = malloc(program->join_tree.num_arcs * 2 * sizeof(uint32_t));
	if (!arc_endpoints)
	{
		snprintf(program->error, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for arc endpoints.");
		goto error;
	}

	// Upload join tree arc index data:
	uint32_t arc_index = 0;
	for (uint32_t i = 0; i < program->join_tree.num_nodes; i++)
	{
		for (uint32_t j = program->join_tree.nodes[i].first_arc[0];
			j < (program->join_tree.nodes[i].first_arc[0] +
			program->join_tree.nodes[i].degree[0]); j++)
		{
			arc_endpoints[arc_index] = i;
			arc_endpoints[arc_index + 1] = program->join_tree.arcs[j];
			arc_index += 2;
		}
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
			&(program->join_arc_buffer), (uint8_t *)(arc_endpoints)))
	{
		goto error;
	}

	// Upload split tree arc index data:
	arc_index = 0;
	for (uint32_t i = 0; i < program->split_tree.num_nodes; i++)
	{
		for (uint32_t j = program->split_tree.nodes[i].first_arc[0];
			j < (program->split_tree.nodes[i].first_arc[0] +
			program->split_tree.nodes[i].degree[0]); j++)
		{
			arc_endpoints[arc_index] = i;
			arc_endpoints[arc_index + 1] = program->split_tree.arcs[j];
			arc_index += 2;
		}
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
			&(program->split_arc_buffer), (uint8_t *)(arc_endpoints)))
	{
		goto error;
	}

	// Upload contour tree arc index data:
	arc_index = 0;
	for (uint32_t i = 0; i < program->contour_tree.num_nodes; i++)
	{
		for (uint32_t j = program->contour_tree.nodes[i].first_arc[0];
			j < (program->contour_tree.nodes[i].first_arc[0] +
			program->contour_tree.nodes[i].degree[0]); j++)
		{
			arc_endpoints[arc_index] = i;
			arc_endpoints[arc_index + 1] = program->contour_tree.arcs[j];
			arc_index += 2;
		}
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
		&(program->contour_arc_buffer), (uint8_t *)(arc_endpoints)))
	{
		goto error;
	}

	free(arc_endpoints);
	arc_endpoints = NULL;

	// Upload object mesh data:
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
		&(program->mesh_buffer_index), (uint8_t *)(gpu_mesh->faces)))
	{
		goto error;
	}
	if (ct_program_upload_helper(program, &staging_allocation, &staging_buffer,
		&(program->mesh_buffer_normals), (uint8_t *)(gpu_mesh->normals)))
	{
		goto error;
	}

	// Cleanup, success:
	if (arc_endpoints) { free(arc_endpoints); }
	if (types) { free(types); }
	if (positions_scalars) { free(positions_scalars); }
	vka_destroy_buffer(&(program->vulkan), &staging_buffer);
	vka_destroy_allocation(&(program->vulkan), &staging_allocation);
	return 0;

	// Cleanup, failure:
	error:
	if (strcmp(program->vulkan.error, "")) { strcpy(program->error, program->vulkan.error); }
	if (arc_endpoints) { free(arc_endpoints); }
	if (types) { free(types); }
	if (positions_scalars) { free(positions_scalars); }
	vka_destroy_buffer(&(program->vulkan), &staging_buffer);
	vka_destroy_allocation(&(program->vulkan), &staging_allocation);
	return -1;
}

int ct_program_upload_helper(ct_program_t *program, vka_allocation_t *staging_allocation,
		vka_buffer_t *staging_buffer, vka_buffer_t *destination, uint8_t *data)
{
	if (vka_begin_command_buffer(&(program->vulkan), &(program->command_buffer))) { return -1; }

	memcpy(staging_allocation->mapped_data, data, destination->size);
	vka_copy_buffer(&(program->command_buffer), staging_buffer, destination);

	if (vka_end_command_buffer(&(program->vulkan), &(program->command_buffer))) { return -1; }
	if (vka_submit_command_buffer(&(program->vulkan), &(program->command_buffer)))
	{
		return -1;;
	}
	if (vka_wait_for_fence(&(program->vulkan), &(program->command_buffer))) { return -1; }

	return 0;
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
		program->node_pipeline.colour_attachment_format = program->vulkan.swapchain_format;
		program->mesh_pipeline.colour_attachment_format = program->vulkan.swapchain_format;

		if (vka_create_pipeline(&(program->vulkan), &(program->node_pipeline)))
		{
			strcpy(program->error, program->vulkan.error);
			return -1;
		}

		if (vka_create_pipeline(&(program->vulkan), &(program->arc_pipeline)))
		{
			strcpy(program->error, program->vulkan.error);
			return -1;
		}

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

	vka_buffer_t vertex_buffers[2];
	vertex_buffers[0].buffer = program->node_buffer_positions_scalars.buffer;
	vertex_buffers[1].buffer = program->mesh_buffer_normals.buffer;
	vka_bind_vertex_buffers(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
						&(program->mesh_buffer_index), 2, vertex_buffers);

	vka_draw_indexed(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
							program->mesh.num_faces * 3, 0, 0);

	vka_bind_pipeline(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->node_pipeline));
	vka_draw(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
						program->join_tree.num_nodes, 0);

	vka_bind_pipeline(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
								&(program->arc_pipeline));
	vka_bind_vertex_buffers(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
					&(program->contour_arc_buffer), 1, vertex_buffers);
	vka_draw_indexed(&(program->vulkan.command_buffers[program->vulkan.current_frame]),
						program->join_tree.num_arcs * 2, 0, 0);

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
