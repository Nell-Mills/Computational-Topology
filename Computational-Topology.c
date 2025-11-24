#include <NM-Config/Config.h>
#include <Mesh-Processing/Mesh-Processing.h>

#include "Source/Computational-Topology.h"

int main(int argc, char **argv)
{
	char error_message[NM_MAX_ERROR_LENGTH];
	strcpy(error_message, "");
	mp_mesh_t mesh = mp_mesh_initialise();
	ct_contour_tree_t contour_tree = ct_contour_tree_initialise();
	ct_vertex_value_t *vertex_values = NULL;

	// Mesh load:
	strcpy(mesh.name, "Test Mesh");
	if (argc > 1) { strcpy(mesh.path, argv[1]); }
	else { strcpy(mesh.path, "Meshes/stanford-bunny.obj"); }
	if (mp_load_obj(&mesh, error_message)) { goto error; }
	if (mp_mesh_calculate_edges(&mesh, error_message)) { goto error; }

	// Scalar value setup:
	vertex_values = malloc(mesh.num_vertices * sizeof(ct_vertex_value_t));
	if (!vertex_values) { goto error; }
	for (uint32_t i = 0; i < mesh.num_vertices; i++)
	{
		vertex_values[i].value = (rand() % mesh.num_vertices) * 1.f;
		vertex_values[i].vertex = i;
	}
	ct_vertex_values_sort(mesh.num_vertices, vertex_values);

	#ifdef MP_DEBUG
	fprintf(stdout, "\n");
	ct_vertex_values_print(stdout, mesh.num_vertices, vertex_values);
	fprintf(stdout, "\n\n");
	mp_mesh_print(stdout, &mesh);
	#endif

	// Cleanup, success:
	free(vertex_values);
	ct_contour_tree_free(&contour_tree);
	mp_mesh_free(&mesh);
	return 0;

	// Cleanup, failure:
	error:
	printf("Error: %s\n", error_message);
	if (vertex_values) { free(vertex_values); }
	ct_contour_tree_free(&contour_tree);
	mp_mesh_free(&mesh);
	return -1;
}
