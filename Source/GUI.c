#include "GUI.h"

int ct_gui_setup(ct_gui_t *gui)
{
	strcpy(gui->vulkan.name, "Contour Tree");
	gui->vulkan.window_resizable = 1;
	gui->vulkan.minimum_window_width = 1080;
	gui->vulkan.minimum_window_height = 720;
	if (vka_setup_vulkan(&(gui->vulkan))) { return -1; }

	strcpy(gui->command_buffer.name, "CT command buffer");
	gui->command_buffer.queue = &(gui->vulkan.graphics_queue);
	if (vka_create_command_buffer(&(gui->vulkan), &(gui->command_buffer))) { return -1; }

	strcpy(gui->mesh_pipeline.name, "CT mesh pipeline");
	strcpy(gui->mesh_pipeline.shaders[VKA_SHADER_TYPE_VERTEX].path, "Shaders/Mesh.vert.sprv");
	strcpy(gui->mesh_pipeline.shaders[VKA_SHADER_TYPE_FRAGMENT].path, "Shaders/Mesh.frag.sprv");
	gui->mesh_pipeline.num_vertex_bindings = 4;
	gui->mesh_pipeline.strides[0] = sizeof(ct_vertex_t);
	gui->mesh_pipeline.strides[1] = sizeof(ct_normal_t);
	gui->mesh_pipeline.strides[2] = sizeof(ct_uv_t);
	gui->mesh_pipeline.strides[3] = sizeof(float); // Scalar value.
	gui->mesh_pipeline.num_vertex_attributes = 4;
	gui->mesh_pipeline.bindings[0] = 0;
	gui->mesh_pipeline.bindings[1] = 1;
	gui->mesh_pipeline.bindings[2] = 2;
	gui->mesh_pipeline.bindings[3] = 3;
	gui->mesh_pipeline.formats[0] = VK_FORMAT_R32G32B32_SFLOAT;
	gui->mesh_pipeline.formats[1] = VK_FORMAT_R8G8B8_SNORM;
	gui->mesh_pipeline.formats[2] = VK_FORMAT_R32G32_SFLOAT;
	gui->mesh_pipeline.formats[3] = VK_FORMAT_R32_SFLOAT;
	gui->mesh_pipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	gui->mesh_pipeline.colour_attachment_format = gui->vulkan.swapchain_format;
	gui->mesh_pipeline.blend_enable = VK_TRUE;
	// TODO depth information.

	if (vka_create_pipeline(&(gui->vulkan), &(gui->mesh_pipeline))) { return -1; }

	// TODO tree pipeline.
	// TODO GUI pipeline.

	gui->mesh_render_info.colour_clear_value.color.float32[0] = 0.5f;
	gui->mesh_render_info.colour_clear_value.color.float32[1] = 0.5f;
	gui->mesh_render_info.colour_clear_value.color.float32[2] = 0.5f;
	gui->mesh_render_info.colour_clear_value.color.float32[3] = 1.f;
	gui->mesh_render_info.colour_load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// TODO depth information.

	strcpy(gui->object_buffers.name, "CT object vertex buffers");
	// TODO object buffers.

	// TODO tree buffers.

	ct_gui_update_window_size(gui);

	return 0;
}

int ct_gui_object_setup(ct_gui_t *gui, ct_mesh_t *mesh, ct_tree_t *join_tree,
			ct_tree_t *split_tree, ct_tree_t *contour_tree)
{
	// Destroy all objects first (file dialogue will be possible in the future):
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->contour_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->split_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->join_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->object_buffers));

	return 0;
}

void ct_gui_shutdown(ct_gui_t *gui)
{
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->contour_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->split_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->join_tree_buffers));
	vka_destroy_vertex_buffers(&(gui->vulkan), &(gui->object_buffers));

	vka_destroy_pipeline(&(gui->vulkan), &(gui->gui_pipeline));
	vka_destroy_pipeline(&(gui->vulkan), &(gui->tree_pipeline));
	vka_destroy_pipeline(&(gui->vulkan), &(gui->mesh_pipeline));

	vka_destroy_command_buffer(&(gui->vulkan), &(gui->command_buffer));
	vka_shutdown_vulkan(&(gui->vulkan));
}

void ct_gui_update_window_size(ct_gui_t *gui)
{
	// Update render info with new swapchain extent:
	gui->mesh_render_info.render_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->mesh_render_info.render_area.extent.height = gui->vulkan.swapchain_extent.height;
	gui->mesh_render_info.render_target_height = gui->vulkan.swapchain_extent.height;
	gui->mesh_render_info.scissor_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->mesh_render_info.scissor_area.extent.height = gui->vulkan.swapchain_extent.height;

	gui->tree_render_info.render_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->tree_render_info.render_area.extent.height = gui->vulkan.swapchain_extent.height;
	gui->tree_render_info.render_target_height = gui->vulkan.swapchain_extent.height;
	gui->tree_render_info.scissor_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->tree_render_info.scissor_area.extent.height = gui->vulkan.swapchain_extent.height;

	gui->gui_render_info.render_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->gui_render_info.render_area.extent.height = gui->vulkan.swapchain_extent.height;
	gui->gui_render_info.render_target_height = gui->vulkan.swapchain_extent.height;
	gui->gui_render_info.scissor_area.extent.width = gui->vulkan.swapchain_extent.width;
	gui->gui_render_info.scissor_area.extent.height = gui->vulkan.swapchain_extent.height;
}

int ct_gui_start_frame(ct_gui_t *gui)
{
	if (vka_get_next_swapchain_image(&(gui->vulkan))) { return -1; }
	if (gui->vulkan.recreate_swapchain)
	{
		if (vka_create_swapchain(&(gui->vulkan))) { return -1; }
		gui->vulkan.recreate_swapchain = 0;
		ct_gui_update_window_size(gui);
	}
	if (gui->vulkan.recreate_pipelines)
	{
		gui->mesh_pipeline.colour_attachment_format = gui->vulkan.swapchain_format;
		gui->tree_pipeline.colour_attachment_format = gui->vulkan.swapchain_format;
		gui->gui_pipeline.colour_attachment_format = gui->vulkan.swapchain_format;

		if (vka_create_pipeline(&(gui->vulkan), &(gui->mesh_pipeline))) { return -1; }
		if (vka_create_pipeline(&(gui->vulkan), &(gui->tree_pipeline))) { return -1; }
		if (vka_create_pipeline(&(gui->vulkan), &(gui->gui_pipeline))) { return -1; }

		gui->vulkan.recreate_pipelines = 0;
	}

	return 0;
}

int ct_gui_render(ct_gui_t *gui)
{
	/* Render contour tree first, depth write ON, depth read ON.
	 * Render mesh next, depth write OFF, depth read ON.
	 * Render GUI last, depth write OFF, depth read OFF. */

	if (vka_begin_command_buffer(&(gui->vulkan),
		&(gui->vulkan.command_buffers[gui->vulkan.current_frame])))
	{
		return -1;
	}

	gui->mesh_render_info.colour_image = &(gui->vulkan.swapchain_images[
				gui->vulkan.current_swapchain_index]);
	gui->mesh_render_info.colour_image_view = &(gui->vulkan.swapchain_image_views[
					gui->vulkan.current_swapchain_index]);

	vka_set_viewport(&(gui->vulkan.command_buffers[gui->vulkan.current_frame]),
							&(gui->mesh_render_info));
	vka_set_scissor(&(gui->vulkan.command_buffers[gui->vulkan.current_frame]),
							&(gui->mesh_render_info));
	vka_begin_rendering(&(gui->vulkan.command_buffers[gui->vulkan.current_frame]),
							&(gui->mesh_render_info));
	vka_end_rendering(&(gui->vulkan.command_buffers[gui->vulkan.current_frame]),
							&(gui->mesh_render_info));

	if (vka_end_command_buffer(&(gui->vulkan),
		&(gui->vulkan.command_buffers[gui->vulkan.current_frame])))
	{
		return -1;
	}

	if (vka_submit_command_buffer(&(gui->vulkan),
		&(gui->vulkan.command_buffers[gui->vulkan.current_frame])) != VK_SUCCESS)
	{
		return -1;
	}

	if (vka_present_image(&(gui->vulkan))) { return -1; }
	vka_next_frame(&(gui->vulkan));

	return 0;
}
