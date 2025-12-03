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
