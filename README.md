<<<<<<< HEAD
# Computational Topology

Implementation of contour tree computation using the "sweep and merge" algorithm, described in the paper [Computing contour trees in all dimensions](https://cdr.lib.unc.edu/concern/articles/sj139b05d).
For debug, define CT_DEBUG when compiling.

## Functionality implemented:

- Vertex ordering by scalar value (simulation of simplicity by index).
- Union find with union by rank and path compression.
- Join, split and contour tree computation using sweep and merge algorithm on triangular meshes.

## Compilation:

Use the provided Makefile (make debug for debug build). Written for Linux.  
To compile without cloning git repos, use make (debug) DEPS_CLONE="".

## Usage:

Run with 1 argument: file path. Only works with .obj meshes at the moment.  
Program will exit if the manifold check fails.

### Credits:

Included meshes obtained from [common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models).
=======
# Mesh Processing

Include Mesh-Processing.h. For debug, define MP_DEBUG when compiling.

## Functionality implemented:
- Mesh loader: currently loads in .obj files using [TinyOBJLoaderC](https://github.com/syoyo/tinyobjloader-c). Can also load voxel data in a restricted format.
- Edge/connectivity information:
    - Vertices (from, to).
    - Next edge in face.
    - Other half edge. Boundary edges have a value of -1 here.
    - Edges are ordered by face, so face index is implicit.
- Manifold check: allows for boundary edges.

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
>>>>>>> mesh-processing/master
