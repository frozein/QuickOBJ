# QuickOBJ
A single-header, cross-platform, simple loader for .obj files and their corresponding .mtl files in under 1000 LOC. Contains only 2 front-facing functions for loading and freeing vertex and material data from a .obj file. Please note that this library does not support every feature a .obj file might contain, it only supports the most simple and neccesary.

Documentation can be found at the top of the file. Make sure to `#define QOBJ_IMPLEMENTATION` in exactly one source file before including the library to compile it.

## Features
- Simple, single function .obj loading
- Automatic vertex indexing
- Automatic mesh grouping by material
