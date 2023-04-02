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

//a 3-dimensional vector of unsigned integers
typedef struct QOBJuvec3
{
	uint32_t v[3];
} QOBJuvec3;

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

	const char* materialName;
	QOBJmaterial material;
} QOBJmesh;

//an error value, returned by all functions which can have errors
typedef enum QOBJerror
{
	QOBJ_SUCCESS = 0,
	QOBJ_ERROR_INVALID_FILE,
	QOBJ_ERROR_IO,
	QOBJ_ERROR_MAX_TOKEN_LEN,
	QOBJ_ERROR_OUT_OF_MEM,
	QOBJ_ERROR_UNSUPPORTED_DATA_TYPE
} QOBJerror;

//----------------------------------------------------------------------//
//FUNCTIONS:

static inline QOBJerror qobj_next_token(FILE* fptr, size_t maxTokenLen, char** token, char* endCh)
{
	char curCh;
	size_t curLen = 0;

	while(true)
	{
		if(curLen >= maxTokenLen)
			return QOBJ_ERROR_MAX_TOKEN_LEN;

		curCh = fgetc(fptr);

		if(curCh == ' ' || curCh == '\n' || curCh == EOF)
			break;

		(*token)[curLen++] = curCh;
	}

	(*token)[curLen] = '\0';
	*endCh = curCh;

	return QOBJ_SUCCESS;
}

static inline QOBJerror qobj_maybe_resize_buffer(void** buffer, size_t elemSize, size_t numElems, size_t* elemCap)
{
	if(numElems < *elemCap)
		return QOBJ_SUCCESS;
	
	*elemCap *= 2;
	*buffer = realloc(*buffer, *elemCap * elemSize);
	if(!*buffer)
		return QOBJ_ERROR_OUT_OF_MEM;

	return QOBJ_SUCCESS;
}

static void qobj_free(size_t numMeshes, QOBJmesh* meshes)
{
	for(int i = 0; i < numMeshes; i++)
	{
		free(meshes[i].vertices);
		free(meshes[i].indices);
		free(meshes[i].materialName)
	}

	if(numMeshes > 0)
		free(meshes);
}

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
	size_t positionSize, normalSize, texCoordSize = 0;
	size_t positionCap, normalCap, texCoordCap = 32;
	QOBJvec3* positions = malloc(positionCap * sizeof(QOBJvec3));
	QOBJvec3* normals   = malloc(normalCap   * sizeof(QOBJvec3));
	QOBJvec2* texCoords = malloc(texCoordCap * sizeof(QOBJvec2));

	*numMeshes = 0;

	const size_t MAX_TOKEN_LEN = 64;
	char* curToken = malloc((MAX_TOKEN_LEN + 1) * sizeof(char));
	char curTokenEnd;

	size_t curMesh = 1;

	while(true)
	{
		QOBJerror tokenError = qobj_next_token(fptr, MAX_TOKEN_LEN, &curToken, &curTokenEnd);
		if(tokenError != QOBJ_SUCCESS)
		{
			errorCode = tokenError;
			goto cleanup;
		}

		if(curToken[0] == '#' || strcmp(curToken, "o") == 0 || strcmp(curToken, "g") == 0) //comment / ignored commands
		{
			if(curTokenEnd == ' ')
				fgets(curToken, MAX_TOKEN_LEN, fptr);
		}
		else if(strcmp(curToken, "v") == 0)
		{
			QOBJvec3 pos;
			fscanf(fptr, "%f %f %f\n", &pos.v[0], &pos.v[1], &pos.v[2]);

			positions[positionSize++] = pos;
			QOBJerror resizeError = qobj_maybe_resize_buffer(&positions, sizeof(QOBJvec3), positionSize, &positionCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "vn") == 0)
		{
			QOBJvec3 normal;
			fscanf(fptr, "%f %f %f\n", &normal.v[0], &normal.v[1], &normal.v[2]);

			normals[normalSize++] = normal;
			QOBJerror resizeError = qobj_maybe_resize_buffer(&normals, sizeof(QOBJvec3), normalSize, &normalCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "vt") == 0)
		{
			QOBJvec2 texCoord;
			fscanf(fptr, "%f %f\n", &texCoord.v[0], &texCoord.v[1]);

			texCoords[texCoordSize++] = texCoord;
			QOBJerror resizeError = qobj_maybe_resize_buffer(&texCoords, sizeof(QOBJvec2), texCoordSize, &texCoordCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "f") == 0)
		{
			QOBJuvec3 vertices[4];

			int32_t i = 0;
			while(true)
			{
				fscanf(fptr, "%d", &vertices[i].v[0]);
				i++;

				char nextCh = fgetc(fptr);
				if(nextCh == ' ')
					continue;
				else if(nextCh == '\n')
					break;
				else
				{
					nextCh = fgetc(fptr);
					if(nextCh == '/')
						fscanf(fptr, "%d", &vertices[i].v[3]);
					else
						ungetc(nextCh, fptr);
					
					fscanf(fptr, "%d", &vertices[i].v[2]);
					
					nextCh = fgetc(fptr);
					if(nextCh == '/')
						fscanf(fptr, "%d", &vertices[i].v[3]);
					else
						ungetc(nextCh, fptr);
				}				
			}
		}
		else if(strcmp(curToken, "usemtl") == 0)
		{
			char materialName[128];
			fgets(materialName, 128, fptr);

			for(curMesh = 0; curMesh < *numMeshes; crMesh++)
				if(strcmp((*meshes)[i].materialName, materialName) == 0)
					break;

			if(curMesh >= numMeshes)
			{
				(*numMeshes)++;
				*meshes = realloc(*meshes, *numMeshes * sizeof(QOBJmesh));
				if(!*meshes)
				{
					errorCode = QOBJ_ERROR_OUT_OF_MEM;
					goto cleanup;
				}

				//TODO: SET MATERIAL
			}
		}
		else if(strcmp(curToken, "mtllib") == 0)
		{
			//TODO
		}
		else
		{
			errorCode = QOBJ_ERROR_UNSUPPORTED_DATA_TYPE;
			goto cleanup;
		}

		if(curTokenEnd == EOF)
			break;
	}

	cleanup: ;

	free(curToken);

	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_free(*numMeshes, *meshes);
		*meshes = NULL;
	}

	free(positions);
	free(normals);
	free(texCoords);

	fclose(fptr);
	return errorCode;
}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QOBJ_H