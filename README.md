# Computational Topology

Implementation of contour tree computation using the "sweep and merge" algorithm.  
For debug, define CT_DEBUG when compiling.

## Functionality implemented:

- Vertex ordering by scalar value.
- Union find on disjoint set data structure.

## Compilation:

Use the provided Makefile (make debug for debug build). Written for Linux.  
To compile without cloning git repos, use make (debug) DEPS_CLONE="".

## Usage:

Run with 1 argument: file path. Only works with .obj meshes at the moment.

### Credits:

Included meshes obtained from [common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models).
