#include <stdio.h>
#include <string.h>

#include <NM-Config/Config.h>

#include "Source/Mesh-Processing.h"

int main(int argc, char **argv)
{
	char error_message[NM_MAX_ERROR_LENGTH];
	strcpy(error_message, "");

	mp_mesh_t mesh = mp_mesh_initialise();
	strcpy(mesh.name, "Test Mesh");
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/stanford-bunny.obj"); }
	if (mp_load_obj(&mesh, error_message)) { goto error; }
	if (mp_mesh_calculate_edges(&mesh, error_message)) { goto error; }

	#ifdef MP_DEBUG
	mp_mesh_print_short(stdout, &mesh);
	fprintf(stdout, "\n");
	#endif

	mp_mesh_free(&mesh);
	return 0;

	error:
	printf("Error: %s\n", error_message);
	mp_mesh_free(&mesh);
	return -1;
}
