#include <stdio.h>
#include <string.h>

#include <NM-Config/Config.h>

#include "Source/Computational-Topology.h"

int main(int argc, char **argv)
{
	ct_program_t program = {0};

	if (argc > 1) { strcpy(program.mesh.path, argv[1]); }
	else { strcpy(program.mesh.path, "Meshes/spot.obj"); }
	strcpy(program.mesh.name, program.mesh.path);

	if (ct_program_setup(&program))
	{
		fprintf(stdout, "\nError: %s\n", program.error);
		return -1;
	}

	if (ct_program_object_setup(&program))
	{
		fprintf(stdout, "\nError: %s\n", program.error);
		ct_program_shutdown(&program);
		return -1;
	}

	// Main loop:
	SDL_Event event;
	int running = 1;
	while (running)
	{
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_EVENT_QUIT) { running = 0; }
		}

		if (ct_program_start_frame(&program)) { break; }
		if (ct_program_render(&program)) { break; }
	}

	ct_program_shutdown(&program);
	return 0;
}
