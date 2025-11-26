#include "Mesh-Loader.h"

int mp_mesh_load(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	if (mp_mesh_load_obj(mesh, error_message))
	{
		mp_mesh_free(mesh);
		return -1;
	}
	if (mp_mesh_calculate_edges(mesh, error_message)) { return -1; }
	if (mp_mesh_check_manifold(mesh, error_message)) { return -1; }
	return 0;
}

int mp_mesh_load_obj(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH])
{
	tinyobj_attrib_t attrib;
	tinyobj_shape_t *shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t *materials = NULL;
	size_t num_materials;

	if (tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials,
		mesh->path, tinyobj_file_reader_callback, NULL, TINYOBJ_FLAG_TRIANGULATE)
		!= TINYOBJ_SUCCESS)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not parse file \"%s\" with TinyOBJLoader-C.", mesh->path);
		return -1;
	}

	if ((attrib.num_vertices) > UINT32_MAX)
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" is too large.", mesh->name);
		tinyobj_free(&attrib, num_shapes, shapes, num_materials, materials);
		return -1;
	}

	mesh->num_vertices = attrib.num_vertices;
	mesh->num_normals = attrib.num_normals;
	mesh->num_colours = attrib.num_normals; // TinyOBJC doesn't use colours, so use normals.
	mesh->num_uv_coordinates = attrib.num_texcoords;
	mesh->num_edges = attrib.num_face_num_verts * 3;
	for (int i = 0; i < NM_MAX_LOD_LEVELS; i++)
	{
		mesh->num_faces[i] = attrib.num_face_num_verts;
	}

	if (!mesh->num_vertices || !mesh->num_faces[0])
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Mesh \"%s\" has no vertices/faces.", mesh->name);
		tinyobj_free(&attrib, num_shapes, shapes, num_materials, materials);
		return -1;
	}

	// If any array is empty, fill it with a single element:
	if (!mesh->num_normals) { mesh->num_normals = 1; }
	if (!mesh->num_colours) { mesh->num_colours = 1; }
	if (!mesh->num_uv_coordinates) { mesh->num_uv_coordinates = 1; }

	if (mp_mesh_allocate(mesh, error_message))
	{
		snprintf(error_message, NM_MAX_ERROR_LENGTH,
			"Could not allocate memory for mesh \"%s\".", mesh->name);
		tinyobj_free(&attrib, num_shapes, shapes, num_materials, materials);
		return -1;
	}

	for (unsigned int i = 0; i < attrib.num_vertices; i++)
	{
		mesh->vertices[i].x = attrib.vertices[i * 3];
		mesh->vertices[i].y = attrib.vertices[(i * 3) + 1];
		mesh->vertices[i].z = attrib.vertices[(i * 3) + 2];
	}

	for (unsigned int i = 0; i < attrib.num_normals; i++)
	{
		mesh->normals[i].x = (int8_t)(attrib.normals[i * 3] * 255);
		mesh->normals[i].y = (int8_t)(attrib.normals[(i * 3) + 1] * 255);
		mesh->normals[i].z = (int8_t)(attrib.normals[(i * 3) + 2] * 255);

		mesh->colours[i].r = (uint8_t)(attrib.normals[i * 3] * 255);
		mesh->colours[i].g = (uint8_t)(attrib.normals[(i * 3) + 1] * 255);
		mesh->colours[i].b = (uint8_t)(attrib.normals[(i * 3) + 2] * 255);
		mesh->colours[i].a = 255;
	}

	for (unsigned int i = 0; i < attrib.num_texcoords; i++)
	{
		mesh->uv_coordinates[i].u = attrib.texcoords[i * 3];
		mesh->uv_coordinates[i].v = attrib.texcoords[(i * 3) + 1];
	}

	for (unsigned int i = 0; i < attrib.num_face_num_verts; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			mesh->faces[0][i].p[j] = attrib.faces[(i * 3) + j].v_idx;
			mesh->faces[0][i].n[j] = 0;
			mesh->faces[0][i].c[j] = 0;
			mesh->faces[0][i].u[j] = 0;
			if (attrib.num_normals > 0)
			{
				mesh->faces[0][i].n[j] = attrib.faces[(i * 3) + j].vn_idx;
				mesh->faces[0][i].c[j] = attrib.faces[(i * 3) + j].vn_idx;
			}
			if (attrib.num_texcoords > 0)
			{
				mesh->faces[0][i].u[j] = attrib.faces[(i * 3) + j].vt_idx;
			}
		}
	}

	tinyobj_free(&attrib, num_shapes, shapes, num_materials, materials);

	return 0;
}

void tinyobj_file_reader_callback(void *ctx, const char *filename, const int is_mtl,
				const char *obj_filename, char **data, size_t *len)
{
	*data = SDL_LoadFile(filename, len);
}

void tinyobj_free(tinyobj_attrib_t *attrib, size_t num_shapes, tinyobj_shape_t *shapes,
				size_t num_materials, tinyobj_material_t *materials)
{
	tinyobj_attrib_free(attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
}
