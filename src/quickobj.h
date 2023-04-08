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
	float v[2];
} QOBJvec2;

//a 3-dimensional vector of floats
typedef struct QOBJvec3
{
	float v[3];
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

//a hashmap with a vec3 of vertex data indices for keys
typedef struct QOBJvertexHashmap
{
	size_t size;
	size_t cap;
	QOBJuvec3* keys;
	uint32_t* vals;
} QOBJvertexHashmap;

//a material (not pbr)
typedef struct QOBJmaterial
{
	int test;
} QOBJmaterial;

//a mesh consisting of vertices and indices
typedef struct QOBJmesh
{
	size_t numVertices;
	size_t vertexCap;
	QOBJvertex* vertices;

	size_t numIndices;
	size_t indexCap;
	uint32_t* indices;

	char* materialName;
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

static QOBJerror qobj_hashmap_create(QOBJvertexHashmap* map)
{
	map->size = 0;
	map->cap = 32;
	map->keys = (QOBJuvec3*)malloc(map->cap * sizeof(QOBJuvec3));
	if(!map->keys)
		return QOBJ_ERROR_OUT_OF_MEM;
	map->vals = (uint32_t*)malloc(map->cap * sizeof(uint32_t));
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
	int32_t found = 0;
	while(map->keys[hash].v[0] != UINT32_MAX)
	{		
		if(map->keys[hash].v[0] == key.v[0] && map->keys[hash].v[1] == key.v[1] && map->keys[hash].v[2] == key.v[2])
		{
			found = 1;
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

		QOBJuvec3* newKeys = (QOBJuvec3*)malloc(map->cap * sizeof(QOBJuvec3));
		if(!newKeys)
			return QOBJ_ERROR_OUT_OF_MEM;
		uint32_t* newVals = (uint32_t*)malloc(map->cap * sizeof(uint32_t));
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
//MESH FUNCTIONS

static QOBJerror qobj_mesh_create(QOBJmesh* mesh, const char* materialName)
{
	mesh->vertexCap = 32;
	mesh->indexCap  = 32;
	mesh->numVertices = 0;
	mesh->numIndices  = 0;

	mesh->vertices = (QOBJvertex*)malloc(mesh->vertexCap * sizeof(QOBJvertex));
	if(!mesh->vertices)
		return QOBJ_ERROR_OUT_OF_MEM;

	mesh->indices = (uint32_t*)malloc(mesh->indexCap * sizeof(uint32_t));
	if(!mesh->indices)
	{
		free(mesh->vertices);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	mesh->materialName = (char*)malloc(strlen(materialName) * sizeof(char));
	strcpy(mesh->materialName, materialName);
	if(!mesh->materialName)
	{
		free(mesh->vertices);
		free(mesh->indices);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	return QOBJ_SUCCESS;
}

static void qobj_mesh_free(QOBJmesh mesh)
{
	free(mesh.vertices);
	free(mesh.indices);
	free(mesh.materialName);
}

//----------------------------------------------------------------------//
//OBJ FUNCTIONS:

static inline QOBJerror qobj_next_token(FILE* fptr, size_t maxTokenLen, char** token, char* endCh)
{
	char curCh;
	size_t curLen = 0;

	while(1)
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
	void* newBuffer = realloc(*buffer, *elemCap * elemSize);
	if(!newBuffer)
		return QOBJ_ERROR_OUT_OF_MEM;
	*buffer = newBuffer;

	return QOBJ_SUCCESS;
}

static void qobj_free(size_t numMeshes, QOBJmesh* meshes)
{
	for(uint32_t i = 0; i < numMeshes; i++)
		qobj_mesh_free(meshes[i]);

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
	size_t positionSize = 0 , normalSize = 0 , texCoordSize = 0;
	size_t positionCap  = 32, normalCap  = 32, texCoordCap  = 32;
	QOBJvec3* positions = (QOBJvec3*)malloc(positionCap * sizeof(QOBJvec3));
	QOBJvec3* normals   = (QOBJvec3*)malloc(normalCap   * sizeof(QOBJvec3));
	QOBJvec2* texCoords = (QOBJvec2*)malloc(texCoordCap * sizeof(QOBJvec2));

	*meshes = (QOBJmesh*)malloc(sizeof(QOBJmesh));
	QOBJvertexHashmap* meshVertexMaps = (QOBJvertexHashmap*)malloc(sizeof(QOBJvertexHashmap));
	*numMeshes = 0;

	const size_t MAX_TOKEN_LEN = 128;
	char* curToken = (char*)malloc(sizeof(char) * MAX_TOKEN_LEN);
	char curTokenEnd;

	size_t curMesh = 1;

	//ensure memory was properly allocated
	if(!positions || !normals || !texCoords || !curToken)
	{
		errorCode = QOBJ_ERROR_OUT_OF_MEM;
		goto cleanup;
	}

	//main loop:
	while(1)
	{
		QOBJerror tokenError = qobj_next_token(fptr, MAX_TOKEN_LEN, &curToken, &curTokenEnd);
		if(tokenError != QOBJ_SUCCESS)
		{
			errorCode = tokenError;
			goto cleanup;
		}

		if(curTokenEnd == EOF)
			break;

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#'         || strcmp(curToken, "o") == 0 || 
		   strcmp(curToken, "g") == 0 || strcmp(curToken, "s") == 0) //comments / ignored commands
		{
			if(curTokenEnd == ' ')
				fgets(curToken, (int32_t)MAX_TOKEN_LEN, fptr);
		}
		else if(strcmp(curToken, "v") == 0)
		{
			QOBJvec3 pos;
			fscanf(fptr, "%f %f %f\n", &pos.v[0], &pos.v[1], &pos.v[2]);

			positions[positionSize++] = pos;
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&positions, sizeof(QOBJvec3), positionSize, &positionCap);
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
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&normals, sizeof(QOBJvec3), normalSize, &normalCap);
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
			QOBJerror resizeError = qobj_maybe_resize_buffer((void**)&texCoords, sizeof(QOBJvec2), texCoordSize, &texCoordCap);
			if(resizeError != QOBJ_SUCCESS)
			{
				errorCode = resizeError;
				goto cleanup;
			}
		}
		else if(strcmp(curToken, "f") == 0)
		{
			QOBJuvec3 vertices[4];

			uint32_t numVertices = 0;
			while(numVertices < 4)
			{
				//read position:
				fscanf(fptr, "%d", &vertices[numVertices].v[0]);

				char nextCh = fgetc(fptr);
				if(nextCh == ' ')
					continue;
				else if(nextCh == '\n')
					break;

				//read normal (if no texture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[numVertices].v[2]);
				else
					ungetc(nextCh, fptr);
				
				//read texture coordinates:
				fscanf(fptr, "%d", &vertices[numVertices].v[1]);
					
				//read normal (if nexture coordinates exist):
				nextCh = fgetc(fptr);
				if(nextCh == '/')
					fscanf(fptr, "%d", &vertices[numVertices].v[2]);
				else
					ungetc(nextCh, fptr);
				
				//break if at end
				nextCh = fgetc(fptr);
				if (nextCh == '\n' || nextCh == EOF)
					break;

				numVertices++;
			}
			numVertices++;

			if(numVertices > 3)
			{
				printf("not working");
				//triangulate quad
			}

			QOBJmesh* mesh = &(*meshes)[curMesh];
			QOBJvertexHashmap* map = &meshVertexMaps[curMesh]; 

			//resize if needed:
			qobj_maybe_resize_buffer((void**)&mesh->indices, sizeof(uint32_t), mesh->numIndices + numVertices, &mesh->indexCap);
			qobj_maybe_resize_buffer((void**)&mesh->vertices, sizeof(QOBJvertex), mesh->numVertices + numVertices, &mesh->vertexCap); //this is potentially wasteful since we dont necessarily add each vertex

			for(uint32_t i = 0; i < numVertices; i++)
			{
				uint32_t indexToAdd = (uint32_t)mesh->numVertices;
				qobj_hashmap_get_or_add(map, vertices[i], &indexToAdd);

				if(indexToAdd == (uint32_t)mesh->numVertices)
				{
					QOBJvertex vertex;
					vertex.pos = positions[vertices[i].v[0]];
					vertex.texCoord = texCoords[vertices[i].v[1]];
					vertex.normal = normals[vertices[i].v[2]];

					mesh->vertices[mesh->numVertices++] = vertex;
				}

				mesh->indices[mesh->numIndices++] = indexToAdd;
			}
		}
		else if(strcmp(curToken, "usemtl") == 0)
		{
			fgets(curToken, (int32_t)MAX_TOKEN_LEN, fptr);

			for(curMesh = 0; curMesh < *numMeshes; curMesh++)
				if(strcmp((*meshes)[curMesh].materialName, curToken) == 0)
					break;

			if(curMesh >= *numMeshes)
			{
				(*numMeshes)++;
				*meshes = (QOBJmesh*)realloc(*meshes, *numMeshes * sizeof(QOBJmesh));
				meshVertexMaps = (QOBJvertexHashmap*)realloc(meshVertexMaps, *numMeshes * sizeof(QOBJvertexHashmap));

				if(!*meshes || !meshVertexMaps)
				{
					errorCode = QOBJ_ERROR_OUT_OF_MEM;
					goto cleanup;
				}

				QOBJerror meshCreateError = qobj_mesh_create(&(*meshes)[*numMeshes - 1], curToken);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					goto cleanup;
				}

				meshCreateError = qobj_hashmap_create(&meshVertexMaps[*numMeshes - 1]);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					goto cleanup;
				}

				//TODO: SET MATERIAL
			}
		}
		else if(strcmp(curToken, "mtllib") == 0)
		{
			fgets(curToken, (int32_t)MAX_TOKEN_LEN, fptr);
			//TODO
		}
		else
		{
			errorCode = QOBJ_ERROR_UNSUPPORTED_DATA_TYPE;
			goto cleanup;
		}
	}

	cleanup: ;

	if(curToken)
		free(curToken);

	for(uint32_t i = 0; i < *numMeshes; i++)
		qobj_hashmap_free(meshVertexMaps[i]);
	if(meshVertexMaps)
		free(meshVertexMaps);

	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_free(*numMeshes, *meshes);
		*numMeshes = 0;
	}

	if(positions)
		free(positions);
	if(normals)
		free(normals);
	if(texCoords)
		free(texCoords);

	fclose(fptr);
	return errorCode;
}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QOBJ_H