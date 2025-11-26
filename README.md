# Mesh Processing

Include Mesh-Processing.h. For debug, define MP_DEBUG when compiling.

## Functionality implemented:
- Mesh loader: currently loads in .obj files using [TinyOBJLoaderC](https://github.com/syoyo/tinyobjloader-c).
- Edge/connectivity information:
    - Vertices (from, to).
    - Next edge in face.
    - Other half edge. Boundary edges have a value of -1 here.
    - Edges are ordered by face, so face index is implicit.
- Manifold check: allows for boundary edges (semicircle vertex neighbourhoods).

## Compilation:
Optionally compile with -fopenmp.  
Link SDL2 with -lSDL2.  
Depends on [Config](https://github.com/Nell-Mills/Config) as well.

## Usage:
Functions return 0 on success, -1 on failure, where applicable.  
To load a mesh, use mp\_mesh\_load().  
To free a mesh, use mp\_mesh\_free().  
To calculate edge information, use mp\_mesh\_calculate\_edges().  
To perform a manifold check, use mp\_mesh\_check\_manifold().  
Note that mesh loading also performs edge calculation and manifold checking.
