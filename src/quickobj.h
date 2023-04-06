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
//HASH MAP FUNCTIONS:

typedef struct QOBJvertexHashmap
{
	size_t size;
	size_t cap;
	QOBJuvec3* keys;
	uint32_t* vals;
} QOBJvertexHashmap;

static QOBJerror qobj_hashmap_create(QOBJvertexHashmap* map)
{
	map->size = 0;
	map->cap = 32;
	map->keys = malloc(map->cap * sizeof(QOBJuvec3));
	if(!map->keys)
		return QOBJ_ERROR_OUT_OF_MEM;
	map->vals = malloc(map->cap * sizeof(uint32_t));
	if(!map->vals)
	{
		free(map->keys);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	memset(map->keys, UINT8_MAX, map->cap * sizeof(QOBJuvec3));

	return QOBJ_SUCCESS;
}

static void qobj_hashmap_free(QOBJvertexHashmap map)
{
	free(map.keys);
	free(map.vals);
}

static inline size_t qobj_hashmap_hash(QOBJuvec3 key)
{
	return 12637 * key.v[0] + 16369 * key.v[1] + 20749 * key.v[2];
}

static QOBJerror qobj_hashmap_get_or_add(QOBJvertexHashmap* map, QOBJuvec3 key, uint32_t* val)
{
	//get hash:
	size_t hash = qobj_hashmap_hash(key) % map->cap;

	//linear probing:
	bool found = false;
	while(map->keys[hash].v[0] != UINT32_MAX)
	{		
		if(map->keys[hash].v[0] = key.v[0] && map->keys[hash].v[1] = key.v[1] && map->keys[hash].v[2] = key.v[2])
		{
			found = true;
			break;
		}

		hash++;
		hash %= map->cap;
	}

	if(found)
		*val = map->vals[hash];
	else
	{
		map->keys[hash] = key;
		map->vals[hash] = *val;
		map->size++;
	}

	//resize and rehash if needed:
	if(map->size >= map->cap / 2)
	{
		size_t oldCap = map->cap;
		map->cap *= 2;

		QOBJuvec3* newKeys = malloc(map->cap * sizeof(QOBJuvec3));
		if(!newKeys)
			return QOBJ_ERROR_OUT_OF_MEM;
		uint32_t* newVals = malloc(map->cal * sizeof(uint32_t));
		if(!newVals)
		{
			free(newKeys);
			return QOBJ_ERROR_OUT_OF_MEM;
		}

		memset(newKeys, UINT8_MAX, map->cap * sizeof(QOBJvec3));

		for(uint32_t i = 0; i < oldCap; i++)
		{
			if(map->keys[i].v[0] == UINT32_MAX)
				continue;

			size_t newHash = qobj_hashmap_hash(map->keys[i]) % map->cap;
			newKeys[newHash] = map->keys[i];
			newVals[newHash] = map->vals[i];
		}

		free(map->keys);
		free(map->vals);
		map->keys = newKeys;
		map->vals = newVals;
	}

	return QOBJ_SUCCESS;
}

//----------------------------------------------------------------------//
//OBJ FUNCTIONS:

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
	for(uint32_t i = 0; i < numMeshes; i++)
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
	if(!positions || !normals || !texCoords)
	{
		errorCode = QOBJ_ERROR_OUT_OF_MEM;
		goto cleanup;
	}

	*numMeshes = 0;

	const size_t MAX_TOKEN_LEN = 128;
	char curToken[MAX_TOKEN_LEN];
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

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#'         || strcmp(curToken, "o") == 0 || 
		   strcmp(curToken, "g") == 0 || strcmp(curToken, "s") == 0) //comments / ignored commands
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
			while(i < 4)
			{
				//read position:
				fscanf(fptr, "%d", &vertices[i].v[0]);

				char nextCh = fgetc(fptr);
				if(nextCh == ' ')
					continue;
				else if(nextCh == '\n')
					break;

				//read normal (if no texture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[i].v[3]);
				else
					ungetc(nextCh, fptr);
				
				//read texture coordinates:
				fscanf(fptr, "%d", &vertices[i].v[2]);
					
				//read normal (if nexture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[i].v[3]);
				else
					ungetc(nextCh, fptr);
				
				//break if at end
				nextCh = fgetc(fptr);
				if(nextCh == '\n')
					break;

				i++;			
			}

			i++;
		}
		else if(strcmp(curToken, "usemtl") == 0)
		{
			fgets(curToken, MAX_TOKEN_LEN, fptr);

			for(curMesh = 0; curMesh < *numMeshes; crMesh++)
				if(strcmp((*meshes)[i].materialName, curToken) == 0)
					break;

			if(curMesh >= *numMeshes)
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