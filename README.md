# Computational Topology

Implementation of contour tree computation using the "sweep and merge" algorithm, described in the paper [Computing contour trees in all dimensions](https://cdr.lib.unc.edu/concern/articles/sj139b05d).

## Functionality:

- Mesh loader: currently loads in .obj files using [TinyOBJLoaderC](https://github.com/syoyo/tinyobjloader-c).
- Edge/connectivity information:
    - Vertices (from, to).
    - Next edge in face.
    - Other half edge. Boundary edges have a value of UINT32_MAX here.
    - Edges are ordered by face, so face index is implicit.
- Manifold check: allows for boundary edges.
- Scalar field preprocessing - sorting by scalar value, with simulation of simplicity by index
- Union find implementation - union by rank, path compression, extremum tracking
- Merge tree construction
- Contour tree construction - leaf-peeling merge of join and split trees.

## Compilation:

Use the provided Makefile (run make, or make debug to have CT_DEBUG defined).  
To build without cloning dependencies, use DEPS_CLONE="".  
Build process has been tested on Linux only.

## Usage:

./Computational-Topology `<path to mesh>`  

Only .obj meshes are currently supported. All loaded meshes are run through a manifold check - the program will halt if this fails.

## Credits:

Included meshes obtained from [common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models).

## Future Work:

- Verification of correctness via comparison with existing implementations - so far, results on small meshes have been verified by hand
- Visualisation of the merge and contour trees in 3D space
- User interaction - changing isovalue, for example
- Simplification of the contour tree (removing regular nodes)
- Export (and possibly import) of merge and contour trees
- Generalisation of contour tree computation to arbitrary dimensions
