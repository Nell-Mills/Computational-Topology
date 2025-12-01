#ifndef MP_MESH_LOADER_H
#define MP_MESH_LOADER_H

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <NM-Config/Config.h>
#include <TinyOBJLoaderC/tinyobj_loader_c.h>
#include <SDL2/SDL_rwops.h>

#include "Mesh.h"

int mp_mesh_load(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);

// OBJ meshes:
int mp_mesh_load_obj(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
void mp_mesh_write_obj(FILE *file, mp_mesh_t *mesh);
void tinyobj_file_reader_callback(void *ctx, const char *filename, const int is_mtl,
				const char *obj_filename, char **data, size_t *len);
void tinyobj_free(tinyobj_attrib_t *attrib, size_t num_shapes, tinyobj_shape_t *shapes,
				size_t num_materials, tinyobj_material_t *materials);

// Voxel meshes:
int mp_mesh_load_voxels(mp_mesh_t *mesh, char error_message[NM_MAX_ERROR_LENGTH]);
uint32_t mp_get_voxel_index(uint32_t coordinates[3], uint32_t dimensions[3]);

#ifdef MP_DEBUG
void mp_voxels_print(FILE *file, uint32_t dimensions[3], float *voxels);
#endif

#endif
