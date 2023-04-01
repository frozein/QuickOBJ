#ifndef QOBJ_H
#define QOBJ_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

//----------------------------------------------------------------------//
//STRUCT DEFINITIONS:

//a 2-dimensional vector of floats
typedef struct QOBJvec2
{
	float v[3];
} QOBJvec2;

//a 3-dimensional vector of floats
typedef struct QOBJvec3
{
	float v[2];
} QOBJvec3;

//a single vertex with position, normal, and texture coordinates
typedef struct QOBJvertex
{
	QOBJvec3 pos;
	QOBJvec3 normal;
	QOBJvec2 texCoord;
} QOBJvertex;

//a material (not pbr)
typedef struct QOBJmaterial
{
	int test;
} QOBJmaterial;

//a mesh consisting of vertices and indices
typedef struct QOBJmesh
{
	size_t numVertices;
	QOBJvertex* vertices;

	size_t numIndices;
	uint32_t* indices;

	QOBJmaterial material;
} QOBJmesh;

//an error value, returned by all functions which can have errors
typedef enum QOBJerror
{
	QOBJ_SUCCESS = 0,
	QOBJ_ERROR_INVALID_FILE,
	QOBJ_ERROR_IO
} QOBJerror;

//----------------------------------------------------------------------//
//FUNCTIONS:

static QOBJerror qobj_load(const char* path, size_t* numMeshes, QOBJmesh** meshes)
{
	//ensure file is valid and able to be opened:
	size_t pathLen = strlen(path);
	if(pathLen < 4 || strcmp(&path[pathLen - 4], ".obj") != 0)
		return QOBJ_ERROR_INVALID_FILE;

	FILE* fptr = fopen(path, "r");
	if(!fptr)
		return QOBJ_ERROR_IO;

	QOBJerror errorCode = QOBJ_SUCCESS;

	//initialize memory:
	size_t positionSize, normalSize, texCoordSize = 32;
	QOBJvec3* positions = malloc(positionSize * sizeof(QOBJvec3));
	QOBJvec3* normals   = malloc(normalSize   * sizeof(QOBJvec3));
	QOBJvec2* texCoords = malloc(texCoordSize * sizeof(QOBJvec2));

	*numMeshes = 1;
	*meshes = malloc(1 * sizeof(QOBJmesh));

	cleanup: ;

	free(positions);
	free(normals);
	free(texCoords);

	fclose(fptr);
	return errorCode;
}

static void qobj_free(QOBJmesh mesh)
{
	free(mesh.vertices);
	free(mesh.indices);
}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QOBJ_H