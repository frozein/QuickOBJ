# QuickOBJ
A single-header, cross-platform, simple loader for .obj files and their corresponding .mtl files ~1000 LOC. Contains only 4 front-facing functions for loading and freeing vertex data material data from `.obj` and `.mtl` files, respectively. Please note that this library does not support every feature a `.obj` file might contain, it only supports the most common features.

Documentation can be found at the top of the file. Make sure to `#define QOBJ_IMPLEMENTATION` in exactly one source file before including the library to compile it.

## Features
- Simple, single function `.obj` and `.mtl` loading
- Automatic vertex trianglulation and indexing
- Automatic mesh grouping by material
