#ifndef MP_MESH_LOADER_H
#define MP_MESH_LOADER_H

#include <stdio.h>

#include <NM-Config/Config.h>
#include <TinyOBJLoaderC/tinyobj_loader_c.h>
#include <SDL2/SDL_rwops.h>

#include "Mesh.h"

int mp_mesh_load(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
int mp_mesh_load_obj(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);

void tinyobj_file_reader_callback(void *ctx, const char *filename, const int is_mtl,
				const char *obj_filename, char **data, size_t *len);
void tinyobj_free(tinyobj_attrib_t *attrib, size_t num_shapes, tinyobj_shape_t *shapes,
				size_t num_materials, tinyobj_material_t *materials);

#endif
