/* ------------------------------------------------------------------------
 * 
 * quickobj.h
 * author: Daniel Elwell (2023)
 * license: MIT
 * description: a single-header library for loading 3D meshes from .obj files, and their
 * associated materials from .mtl files
 * 
 * ------------------------------------------------------------------------
 * 
 * to use you must "#define QOBJ_IMPLEMENTATION" in exactly one source file before including the library
 * 
 * if you wish to use a custom memory allocator instead of the default malloc(), you must 
 * "#define QOBJ_MALLOC(s) my_malloc(s)", "#define QOBJ_FREE(p) my_free(p)", and "#define QOBJ_REALLOC(p, s) my_realloc(p, s)"
 * before including the library in the same source file you used "#define QOBJ_IMPLEMENTATION"
 * 
 * the following strutures, enums, and functions are defined for end use:
 * (all other functions/structures are meant for internal library use only and do not have documentation)
 * 
 * STRUCTURES:
 * ------------------------------------------------------------------------
 * QOBJcolor
 * 		an RGB color
 * 
 * QOBJmaterial
 * 		a single material (non-PBR)
 * 		contains:
 * 			ambient color (vec3); diffuse color (vec3); specular color (vec3)
 * 			ambient color map (char*); diffuse color map (char*); specular color map (char*); normal map (char*)
 * 			opacity (float); shininess/specular exponent (float); refraction index (float)
 * 
 * 		NOTE: if a map is NULL, then it does not exist, otherwise, it contains the path to the texture
 * 
 * QOBJmesh
 * 		a mesh with a single material, uses an index buffer
 * 		meshesa are triangulated, so every group of 3 indices defines a triangle
 * 		contains:
 * 			vertex attributes (QOBJvertexAttributes) (see enum definition)
 * 			vertex stride (uint32_t) (the number of floats each vertex takes)
 * 			vertex pos offset (uint32_t) (the offset of the position attribute in floats, if it exists)
 * 			vertex normal offset (uint32_t) (the offset of the normal attribute in floats, if it exsts)
 * 			vertex tex coord offset (uint32_t) (the offset of the tex coord attribute in floats, if it exsts)
 * 
 * 			number of vertices (uint32_t); vertex buffer capacity (uint32_t) (for internal use, please ignore)
 * 			array of vertices (float*)
 * 			
 * 			number of indices (uint32_t); index buffer capacity (uint32_t) (for internal use, please ignore)
 * 			array of indices (uint32_t*)
 * 
 * 			material name (char*)
 * 
 * ENUMS:
 * ------------------------------------------------------------------------
 * QOBJerror
 * 		the return value for most funtions, defines any possible errors that could occur
 * 
 * QOBJvertexAttributes
 * 		all possible attributes that a vertex could have, a mesh's vertex attributes will be an OR of 1 or more of these
 * 
 * FUNCTIONS:
 * ------------------------------------------------------------------------
 * QOBJerror qobj_load_obj(const char* path, uint32_t* numMeshes, QOBJmesh** meshes)
 * 		loads a .obj file from [path]
 * 		the [numMeshes] field is populated with the number of meshes loaded
 * 		the [meshes] field is populated with all of the loaded meshes
 * 		NOTE: in order to render the entire model, you must render each mesh in the array, using its corresponding material (loaded separately)
 * 
 * void qobj_free_obj(uint32_t numMeshes, QOBJmesh* meshes)
 * 		frees the memory created by a call to qobj_load_obj, must be called in order to prevent memory leaks
 * 
 * QOBJerror qobj_load_mtl(const char* path, uint32_t* numMaterials, QOBJmaterial** materials)
 * 		loads a .mtl file from [path]
 * 		the [numMaterials] field is populated with the number of materials loaded
 * 		the [materials] field is populated with all of the loaded materials
 * 
 * void qobj_free_mtl(uint32_t numMaterials, QOBJmaterial* materials)
 * 		frees the memory created by a call to qobj_load_mtl, must be called in order to prevent memory leaks
 */

#ifndef QOBJ_H
#define QOBJ_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>

//----------------------------------------------------------------------//
//DECLARATIONS:

//an RGB color
typedef struct QOBJcolor
{
	float r, g, b;
} QOBJcolor;

//a material (not pbr)
typedef struct QOBJmaterial
{
	char* name;

	QOBJcolor ambientColor;
	QOBJcolor diffuseColor;
	QOBJcolor specularColor;
	char* ambientMapPath;  //== NULL if one does not exist
	char* diffuseMapPath;  //== NULL if one does not exist
	char* specularMapPath; //== NULL if one does not exist
	char* normalMapPath;   //== NULL if one does not exist

	float opacity;
	float specularExp;
	float refractionIndex;
} QOBJmaterial;

//a mesh consisting of vertices and indices, contains only triangles
typedef struct QOBJmesh
{
	uint32_t vertexAttribs;        //bitfield of QOBJvertexAttributes
	uint32_t vertexStride;         //size of each vertex in number of floats
	uint32_t vertexPosOffset;      //offset of the position attribute in number of floats (or UINT32_MAX if no positions given)
	uint32_t vertexNormalOffset;   //offset of the normal attribute in number of floats (or UINT32_MAX if no normals given)
	uint32_t vertexTexCoordOffset; //offset of the texture coordinate attribute in number of floats (or UINT32_MAX if no tex coords given)

	uint32_t numVertices;
	uint32_t vertexCap;
	float* vertices;

	uint32_t numIndices; //mesh only contains triangles, so the number of tris is numIndices / 3
	uint32_t indexCap;
	uint32_t* indices;

	char* material;
} QOBJmesh;

//an error value, returned by all functions which can have errors
typedef enum QOBJerror
{
	QOBJ_SUCCESS = 0,
	QOBJ_ERROR_INVALID_FILE,
	QOBJ_ERROR_IO,
	QOBJ_ERROR_OUT_OF_MEM,
	QOBJ_ERROR_UNSUPPORTED_DATA_TYPE
} QOBJerror;

//different attributes that the vertices within a mesh can have
typedef enum QOBJvertexAttributes
{
	QOBJ_VERTEX_ATTRIB_POSITION   = (1 << 0),
	QOBJ_VERTEX_ATTRIB_NORMAL     = (1 << 1),
	QOBJ_VERTEX_ATTRIB_TEX_COORDS = (1 << 2)
} QOBJvertexAttributes;

//loads all meshes from a valid .obj file
QOBJerror qobj_load_obj(const char* path, uint32_t* numMeshes, QOBJmesh** meshes);
//frees all resources allocated from qobj_load_obj()
void qobj_free_obj(uint32_t numMeshes, QOBJmesh* meshes);

//loads all materials from a valid .mtl file
QOBJerror qobj_load_mtl(const char* path, uint32_t* numMaterials, QOBJmaterial** materials);
//frees all resources allocated from qobj_load_mtl()
void qobj_free_mtl(uint32_t numMaterials, QOBJmaterial* materials);

//----------------------------------------------------------------------//

#ifdef QOBJ_IMPLEMENTATION

#include <stdio.h>
#include <string.h>

#if !defined(QOBJ_MALLOC) || !defined(QOBJ_FREE) || !defined(QOBJ_FREE)
	#include <malloc.h>

	#define QOBJ_MALLOC(s) malloc(s)
	#define QOBJ_FREE(p) free(p)
	#define QOBJ_REALLOC(p, s) realloc(p, s)
#endif

#define QOBJ_ATTRIB_SIZE_POSITION   3
#define QOBJ_ATTRIB_SIZE_NORMAL     3
#define QOBJ_ATTRIB_SIZE_TEX_COORDS 2

//----------------------------------------------------------------------//
//IMPLEMENTATION STRUCTS/ENUMS:

//a reference to a vertex (specified in the "f" command in an OBJ file)
typedef struct QOBJvertexRef
{
	uint32_t pos;
	uint32_t normal;
	uint32_t texCoord;
} QOBJvertexRef;

//a hashmap with a vec3 of vertex data indices for keys
typedef struct QOBJvertexHashmap
{
	uint32_t size;
	uint32_t cap;
	QOBJvertexRef* keys; //an x component of UINT32_MAX signifies an unused index
	uint32_t* vals;
} QOBJvertexHashmap;

//valid combinations of vertices a mesh can have, used for reading vertices in different formats
typedef enum QOBJvertexSpecification
{
	QOBJ_VERTEX_SPEC_POSITION                  = QOBJ_VERTEX_ATTRIB_POSITION,
	QOBJ_VERTEX_SPEC_POSITION_TEX_COORD        = QOBJ_VERTEX_ATTRIB_POSITION | QOBJ_VERTEX_ATTRIB_TEX_COORDS,
	QOBJ_VERTEX_SPEC_POSITION_NORMAL           = QOBJ_VERTEX_ATTRIB_POSITION | QOBJ_VERTEX_ATTRIB_NORMAL,
	QOBJ_VERTEX_SPEC_POSITION_TEX_COORD_NORMAL = QOBJ_VERTEX_ATTRIB_POSITION | QOBJ_VERTEX_ATTRIB_TEX_COORDS | QOBJ_VERTEX_ATTRIB_NORMAL,
} QOBJvertexSpecification;

//----------------------------------------------------------------------//
//GENERAL HELPER FUNCTIONS:

#define QOBJ_MAX_TOKEN_LEN 128

inline void qobj_next_token(FILE* fptr, char* token, char* endCh)
{
	char curCh;
	uint32_t curLen = 0;

	while(1)
	{
		if(curLen >= QOBJ_MAX_TOKEN_LEN)
		{
			curLen--;
			break;
		}

		curCh = fgetc(fptr);

		if(isspace(curCh) || curCh == EOF)
			break;

		token[curLen++] = curCh;
	}

	token[curLen] = '\0';
	*endCh = curCh;
}

inline void qobj_fgets(FILE* fptr, char* token, char* endCh)
{
	fgets(token, QOBJ_MAX_TOKEN_LEN, fptr);

	uint32_t last = (uint32_t)strlen(token) - 1;
	if(token[last] == '\n')
	{
		*endCh = token[last];
		token[last] = '\0';
	}
	else
		*endCh = EOF;
}

inline QOBJerror qobj_maybe_resize_array(void** buffer, size_t elemSize, uint32_t numElems, uint32_t* elemCap)
{
	if(numElems < *elemCap)
		return QOBJ_SUCCESS;
	
	*elemCap *= 2;
	void* newBuffer = QOBJ_REALLOC(*buffer, *elemCap * elemSize);
	if(!newBuffer)
		return QOBJ_ERROR_OUT_OF_MEM;
	*buffer = newBuffer;

	return QOBJ_SUCCESS;
}

//----------------------------------------------------------------------//
//HASH MAP FUNCTIONS:

QOBJerror qobj_hashmap_create(QOBJvertexHashmap* map)
{
	map->size = 0;
	map->cap = 32;
	map->keys = (QOBJvertexRef*)calloc(map->cap, sizeof(QOBJvertexRef));
	if(!map->keys)
		return QOBJ_ERROR_OUT_OF_MEM;

	map->vals = (uint32_t*)QOBJ_MALLOC(map->cap * sizeof(uint32_t));
	if(!map->vals)
	{
		QOBJ_FREE(map->keys);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	return QOBJ_SUCCESS;
}

void qobj_hashmap_free(QOBJvertexHashmap map)
{
	QOBJ_FREE(map.keys);
	QOBJ_FREE(map.vals);
}

inline size_t qobj_hashmap_hash(QOBJvertexRef key)
{
	return 12637 * key.pos + 16369 * key.normal + 20749 * key.texCoord;
}

QOBJerror qobj_hashmap_get_or_add(QOBJvertexHashmap* map, QOBJvertexRef key, uint32_t* val)
{
	//get hash:
	//---------------
	size_t hash = qobj_hashmap_hash(key) % map->cap;

	//linear probing:
	//---------------
	int32_t found = 0;
	while(map->keys[hash].pos != 0)
	{		
		if(map->keys[hash].pos == key.pos && map->keys[hash].normal == key.normal && map->keys[hash].texCoord == key.texCoord)
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
	//---------------
	if(map->size >= map->cap / 2)
	{
		uint32_t oldCap = map->cap;
		map->cap *= 2;

		QOBJvertexRef* newKeys = (QOBJvertexRef*)calloc(map->cap, sizeof(QOBJvertexRef));
		if(!newKeys)
			return QOBJ_ERROR_OUT_OF_MEM;
		uint32_t* newVals = (uint32_t*)QOBJ_MALLOC(map->cap * sizeof(uint32_t));
		if(!newVals)
		{
			QOBJ_FREE(newKeys);
			return QOBJ_ERROR_OUT_OF_MEM;
		}

		for(uint32_t i = 0; i < oldCap; i++)
		{
			if(map->keys[i].pos == 0)
				continue;

			size_t newHash = qobj_hashmap_hash(map->keys[i]) % map->cap;
			newKeys[newHash] = map->keys[i];
			newVals[newHash] = map->vals[i];
		}

		QOBJ_FREE(map->keys);
		QOBJ_FREE(map->vals);
		map->keys = newKeys;
		map->vals = newVals;
	}

	return QOBJ_SUCCESS;
}

//----------------------------------------------------------------------//
//MESH FUNCTIONS

QOBJerror qobj_mesh_create(QOBJmesh* mesh, uint32_t vertexAttribs, const char* materialName)
{
	//determine strides and offsets of attributes:
	//---------------
	mesh->vertexAttribs = vertexAttribs;
	mesh->vertexStride = 0;

	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_POSITION)
	{
		mesh->vertexPosOffset = mesh->vertexStride;
		mesh->vertexStride += QOBJ_ATTRIB_SIZE_POSITION;
	}
	else
		mesh->vertexPosOffset = UINT32_MAX;

	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_NORMAL)
	{
		mesh->vertexNormalOffset = mesh->vertexStride;
		mesh->vertexStride += QOBJ_ATTRIB_SIZE_NORMAL;
	}
	else
		mesh->vertexNormalOffset = UINT32_MAX;
	
	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_TEX_COORDS)
	{
		mesh->vertexTexCoordOffset = mesh->vertexStride;
		mesh->vertexStride += QOBJ_ATTRIB_SIZE_TEX_COORDS;
	}
	else
		mesh->vertexTexCoordOffset = UINT32_MAX;

	//allocate data:
	//---------------
	mesh->vertexCap   = 32;
	mesh->indexCap    = 32;
	mesh->numVertices = 0;
	mesh->numIndices  = 0;

	mesh->vertices = (float*)QOBJ_MALLOC(mesh->vertexCap * sizeof(float) * mesh->vertexStride);
	if(!mesh->vertices)
		return QOBJ_ERROR_OUT_OF_MEM;

	mesh->indices = (uint32_t*)QOBJ_MALLOC(mesh->indexCap * sizeof(uint32_t));
	if(!mesh->indices)
	{
		QOBJ_FREE(mesh->vertices);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	//copy material name:
	//---------------
	uint32_t nameSize = (uint32_t)strlen(materialName) + 1;
	mesh->material = (char*)QOBJ_MALLOC(nameSize);
	if(!mesh->material)
	{
		QOBJ_FREE(mesh->vertices);
		QOBJ_FREE(mesh->indices);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	strcpy_s(mesh->material, nameSize, materialName);

	return QOBJ_SUCCESS;
}

void qobj_mesh_free(QOBJmesh mesh)
{
	QOBJ_FREE(mesh.vertices);
	QOBJ_FREE(mesh.indices);
	QOBJ_FREE(mesh.material);
}

//----------------------------------------------------------------------//
//MATERIAL FUNCTIONS:

QOBJmaterial qobj_default_material()
{
	QOBJmaterial result = {0};

	result.opacity = 1.0f;
	result.specularExp = 1.0f;
	result.refractionIndex = 1.0f;

	return result;
}

void qobj_material_free(QOBJmaterial material)
{
	if(material.name)
		QOBJ_FREE(material.name);

	if(material.ambientMapPath)
		QOBJ_FREE(material.ambientMapPath);
	if(material.diffuseMapPath)
		QOBJ_FREE(material.diffuseMapPath);
	if(material.specularMapPath)
		QOBJ_FREE(material.specularMapPath);
	if(material.normalMapPath)
		QOBJ_FREE(material.normalMapPath);
}

//----------------------------------------------------------------------//
//VERTEX HELPER FUNCTION:

inline int32_t qobj_read_vertex_ref(FILE* fptr, QOBJvertexSpecification spec, QOBJvertexRef* vert)
{
	int32_t numRead;

	switch(spec)
	{
	case QOBJ_VERTEX_SPEC_POSITION:
	{
		numRead = fscanf_s(fptr, "%d", &vert->pos);
		vert->texCoord = 0;
		vert->normal = 0;
		break;
	}
	case QOBJ_VERTEX_SPEC_POSITION_TEX_COORD:
	{
		numRead = fscanf_s(fptr, "%d/%d", &vert->pos, &vert->texCoord);
		vert->normal = 0;
		break;
	}
	case QOBJ_VERTEX_SPEC_POSITION_NORMAL:
	{
		numRead = fscanf_s(fptr, "%d//%d", &vert->pos, &vert->normal);
		vert->texCoord = 0;
		break;
	}
	case QOBJ_VERTEX_SPEC_POSITION_TEX_COORD_NORMAL:
	{
		numRead = fscanf_s(fptr, "%d/%d/%d", &vert->pos, &vert->texCoord, &vert->normal);
		break;
	}
	}

	return numRead;
}

inline void qobj_add_vertex(QOBJmesh* mesh, QOBJvertexHashmap* map, QOBJvertexRef vert,
                            float* positions, float* texCoords, float* normals)
{
	uint32_t indexToAdd = (uint32_t)mesh->numVertices;
	qobj_hashmap_get_or_add(map, vert, &indexToAdd);

	mesh->indices[mesh->numIndices++] = indexToAdd;
	if(indexToAdd < (uint32_t)mesh->numVertices) //skip if vertex already exists in mesh
		return;

	uint32_t insertIdx = (uint32_t)mesh->numVertices++ * mesh->vertexStride;

	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_POSITION && vert.pos > 0)
	{
		uint32_t posIdx = (vert.pos - 1) * QOBJ_ATTRIB_SIZE_POSITION; //.obj files are 1-indexed

		mesh->vertices[insertIdx + mesh->vertexPosOffset + 0] = positions[posIdx + 0];
		mesh->vertices[insertIdx + mesh->vertexPosOffset + 1] = positions[posIdx + 1];
		mesh->vertices[insertIdx + mesh->vertexPosOffset + 2] = positions[posIdx + 2];
	}

	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_NORMAL && vert.normal > 0)
	{
		uint32_t normalIdx = (vert.normal - 1) * QOBJ_ATTRIB_SIZE_NORMAL; //.obj files are 1-indexed

		mesh->vertices[insertIdx + mesh->vertexNormalOffset + 0] = normals[normalIdx + 0];
		mesh->vertices[insertIdx + mesh->vertexNormalOffset + 1] = normals[normalIdx + 1];
		mesh->vertices[insertIdx + mesh->vertexNormalOffset + 2] = normals[normalIdx + 2];
	}

	if(mesh->vertexAttribs & QOBJ_VERTEX_ATTRIB_TEX_COORDS && vert.texCoord > 0)
	{
		uint32_t texCoordIdx = (vert.texCoord - 1) * QOBJ_ATTRIB_SIZE_TEX_COORDS; //.obj files are 1-indexed

		mesh->vertices[insertIdx + mesh->vertexTexCoordOffset + 0] = texCoords[texCoordIdx + 0];
		mesh->vertices[insertIdx + mesh->vertexTexCoordOffset + 1] = texCoords[texCoordIdx + 1];
	}
}

inline QOBJerror qobj_add_triangle(QOBJmesh* mesh, QOBJvertexHashmap* map, QOBJvertexRef v0, QOBJvertexRef v1, QOBJvertexRef v2,
                                   float* positions, float* texCoords, float* normals)
{
	//resize buffers if needed:
	//---------------
	QOBJerror resizeError = qobj_maybe_resize_array((void**)&mesh->indices, sizeof(uint32_t), mesh->numIndices + 3, &mesh->indexCap);
	if(resizeError != QOBJ_SUCCESS)
		return resizeError;

	resizeError = qobj_maybe_resize_array((void**)&mesh->vertices, sizeof(float) * mesh->vertexStride, mesh->numVertices + 3, &mesh->vertexCap);
	if(resizeError != QOBJ_SUCCESS)
		return resizeError;

	//add vertices + indices:
	//---------------
	qobj_add_vertex(mesh, map, v0, positions, texCoords, normals);
	qobj_add_vertex(mesh, map, v1, positions, texCoords, normals);
	qobj_add_vertex(mesh, map, v2, positions, texCoords, normals);

	//return:
	return QOBJ_SUCCESS;
}

//----------------------------------------------------------------------//
//OBJ LOAD FUNCTION:

QOBJerror qobj_load_obj(const char* path, uint32_t* numMeshes, QOBJmesh** meshes)
{
	*numMeshes = 0;
	*meshes = NULL;

	//ensure file is valid and able to be opened:
	//---------------
	uint32_t pathLen = (uint32_t)strlen(path);
	if(pathLen < 4 || strcmp(&path[pathLen - 4], ".obj") != 0)
		return QOBJ_ERROR_INVALID_FILE;

	FILE* fptr;
	if(fopen_s(&fptr, path, "r") != 0)
		return QOBJ_ERROR_IO;

	//allocate memory:
	//---------------
	uint32_t positionSize = 0 , normalSize = 0 , texCoordSize = 0;
	uint32_t positionCap  = 32, normalCap  = 32, texCoordCap  = 32;
	float* positions = (float*)QOBJ_MALLOC(positionCap * sizeof(float) * QOBJ_ATTRIB_SIZE_POSITION);
	float* normals   = (float*)QOBJ_MALLOC(normalCap   * sizeof(float) * QOBJ_ATTRIB_SIZE_NORMAL);
	float* texCoords = (float*)QOBJ_MALLOC(texCoordCap * sizeof(float) * QOBJ_ATTRIB_SIZE_TEX_COORDS);

	*numMeshes = 0;
	*meshes = (QOBJmesh*)QOBJ_MALLOC(sizeof(QOBJmesh));
	QOBJvertexHashmap* meshVertexMaps = (QOBJvertexHashmap*)QOBJ_MALLOC(sizeof(QOBJvertexHashmap));

	//ensure memory was properly allocated:
	//---------------
	if(!positions || !normals || !texCoords || !*meshes || !meshVertexMaps)
	{
		QOBJ_FREE(positions);
		QOBJ_FREE(normals);
		QOBJ_FREE(texCoords);

		QOBJ_FREE(meshes);
		QOBJ_FREE(meshVertexMaps);

		fclose(fptr);

		return QOBJ_ERROR_OUT_OF_MEM;
	}

	//main loop:
	//---------------
	QOBJerror errorCode = QOBJ_SUCCESS;

	char curToken[QOBJ_MAX_TOKEN_LEN];
	char curTokenEnd;

	char curMaterial[QOBJ_MAX_TOKEN_LEN] = {}; //no material specified (yet)
	uint32_t curMesh = UINT32_MAX;             //no working mesh (yet)

	while(1)
	{
		qobj_next_token(fptr, curToken, &curTokenEnd);

		if(curTokenEnd == EOF)
			break;

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#'         || strcmp(curToken, "o") == 0 || 
		   strcmp(curToken, "g") == 0 || strcmp(curToken, "s") == 0 || //comments / ignored commands
		   strcmp(curToken, "mtllib") == 0)
		{
			if(isspace(curTokenEnd))
				qobj_fgets(fptr, curToken, &curTokenEnd);
		}
		else if(strcmp(curToken, "v") == 0)
		{
			uint32_t insertIdx = (uint32_t)positionSize++ * QOBJ_ATTRIB_SIZE_POSITION;

			fscanf_s(fptr, "%f %f %f\n", 
				&positions[insertIdx + 0], 
				&positions[insertIdx + 1], 
				&positions[insertIdx + 2]
			);

			errorCode = qobj_maybe_resize_array((void**)&positions, sizeof(float) * QOBJ_ATTRIB_SIZE_POSITION, positionSize, &positionCap);
			if(errorCode != QOBJ_SUCCESS)
				break;
		}
		else if(strcmp(curToken, "vn") == 0)
		{
			uint32_t insertIdx = (uint32_t)normalSize++ * QOBJ_ATTRIB_SIZE_NORMAL;

			fscanf_s(fptr, "%f %f %f\n", 
				&normals[insertIdx + 0], 
				&normals[insertIdx + 1], 
				&normals[insertIdx + 2]
			);

			errorCode = qobj_maybe_resize_array((void**)&normals, sizeof(float) * QOBJ_ATTRIB_SIZE_NORMAL, normalSize, &normalCap);
			if(errorCode != QOBJ_SUCCESS)
				break;
		}
		else if(strcmp(curToken, "vt") == 0)
		{
			//TODO: v is optional, account for this

			uint32_t insertIdx = (uint32_t)texCoordSize++ * QOBJ_ATTRIB_SIZE_TEX_COORDS;

			fscanf_s(fptr, "%f %f\n", 
				&texCoords[insertIdx + 0], 
				&texCoords[insertIdx + 1]
			);

			errorCode = qobj_maybe_resize_array((void**)&texCoords, sizeof(float) * QOBJ_ATTRIB_SIZE_TEX_COORDS, texCoordSize, &texCoordCap);
			if(errorCode != QOBJ_SUCCESS)
				break;
		}
		else if(strcmp(curToken, "f") == 0)
		{
			//read first vertex + determine format:
			//---------------
			QOBJvertexRef firstVertex;

			QOBJvertexSpecification spec;
			uint32_t numRead = fscanf_s(fptr, "%d/%d/%d", &firstVertex.pos, &firstVertex.texCoord, &firstVertex.normal);
			
			switch(numRead)
			{
			case 1:
			{
				char nextCh = fgetc(fptr);
				if(nextCh == '/')
				{
					fscanf_s(fptr, "%d", &firstVertex.normal);

					spec = QOBJ_VERTEX_SPEC_POSITION_NORMAL;	
					firstVertex.texCoord = 0;
				}
				else
				{
					spec = QOBJ_VERTEX_SPEC_POSITION;
					firstVertex.texCoord = 0;
					firstVertex.normal = 0;
				}

				break;
			}
			case 2:
			{
				spec = QOBJ_VERTEX_SPEC_POSITION_TEX_COORD;
				firstVertex.normal = 0;
				break;
			}
			case 3:
			{
				spec = QOBJ_VERTEX_SPEC_POSITION_TEX_COORD_NORMAL;
				break;
			}
			case 0:
			default: //nothing read, "f" exists with no params
			{
				spec = (QOBJvertexSpecification)0;
				break;
			}
			}

			if(spec == 0)
			{
				errorCode = QOBJ_ERROR_INVALID_FILE;
				break;
			}

			//if no mesh is active yet, try to find an existing mesh with the same material:
			//---------------
			if(curMesh == UINT32_MAX)
				for(uint32_t i = 0; i < *numMeshes; i++)
				{
					if(strcmp(curMaterial, meshes[i]->material) == 0)
					{
						curMesh = i;
						break;
					}
				}

			//if no valid active mesh was found, create a new one:
			//---------------
			if(curMesh == UINT32_MAX)
			{
				//set curMesh:
				curMesh = (uint32_t)*numMeshes;

				//allocate mem and create new mesh:
				QOBJmesh* newMeshes = (QOBJmesh*)QOBJ_REALLOC(*meshes, (*numMeshes + 1) * sizeof(QOBJmesh));
				if(!newMeshes)
				{
					errorCode = QOBJ_ERROR_OUT_OF_MEM;
					break;
				}
				else
					*meshes = newMeshes;

				QOBJvertexHashmap* newMeshVertexMaps = (QOBJvertexHashmap*)QOBJ_REALLOC(meshVertexMaps, (*numMeshes + 1) * sizeof(QOBJvertexHashmap));
				if(!newMeshVertexMaps)
				{
					errorCode = QOBJ_ERROR_OUT_OF_MEM;
					break;
				}
				else
					meshVertexMaps = newMeshVertexMaps;

				//every spec is a valid set of attribs, see definition
				QOBJerror meshCreateError = qobj_mesh_create(&(*meshes)[curMesh], spec, curMaterial);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					errorCode = meshCreateError;
					break;
				}

				meshCreateError = qobj_hashmap_create(&meshVertexMaps[curMesh]);
				if(meshCreateError != QOBJ_SUCCESS)
				{
					qobj_mesh_free((*meshes)[curMesh]);

					errorCode = meshCreateError;
					break;
				}

				//increment num meshes:
				(*numMeshes)++;
			}

			//read next 2 vertices:
			//---------------
			QOBJvertexRef v1, v2;

			if(qobj_read_vertex_ref(fptr, spec, &v1) == 0 || qobj_read_vertex_ref(fptr, spec, &v2) == 0)
			{
				errorCode = QOBJ_ERROR_INVALID_FILE;
				break;
			}

			//add vertices to mesh and continue reading, triangulating face:
			//---------------
			QOBJmesh* mesh = &(*meshes)[curMesh];
			QOBJvertexHashmap* map = &meshVertexMaps[curMesh]; 

			while(1)
			{
				errorCode = qobj_add_triangle(mesh, map, firstVertex, v1, v2, positions, texCoords, normals);
				if(errorCode != QOBJ_SUCCESS)
					break;
			
				v1 = v2;

				int32_t readResult = qobj_read_vertex_ref(fptr, spec, &v2);
				if(readResult == 0 || readResult == EOF)
					break;
			}

			if(errorCode != QOBJ_SUCCESS)
				break;
		}
		else if(strcmp(curToken, "usemtl") == 0)
		{
			qobj_fgets(fptr, curToken, &curTokenEnd);
			
			strcpy_s(curMaterial, QOBJ_MAX_TOKEN_LEN, curToken);
			curMesh = UINT32_MAX;
		}
		else
		{
			errorCode = QOBJ_ERROR_UNSUPPORTED_DATA_TYPE;
			break;
		}
	}

	//cleanup:
	//---------------
	for(uint32_t i = 0; i < *numMeshes; i++)
		qobj_hashmap_free(meshVertexMaps[i]);

	QOBJ_FREE(meshVertexMaps);

	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_free_obj(*numMeshes, *meshes);
		*numMeshes = 0;
	}

	QOBJ_FREE(positions);
	QOBJ_FREE(normals);
	QOBJ_FREE(texCoords);

	fclose(fptr);
	return errorCode;
}

void qobj_free_obj(uint32_t numMeshes, QOBJmesh* meshes)
{
	if(meshes == NULL)
		return;

	for(uint32_t i = 0; i < numMeshes; i++)
		qobj_mesh_free(meshes[i]);
	
	QOBJ_FREE(meshes);
}

//----------------------------------------------------------------------//
//MTL LOAD FUNCTIONS:

QOBJerror qobj_load_mtl(const char* path, uint32_t* numMaterials, QOBJmaterial** materials)
{
	//ensure file is valid and able to be opened:
	//---------------
	uint32_t pathLen = (uint32_t)strlen(path);
	if(pathLen < 4 || strcmp(&path[pathLen - 4], ".mtl") != 0)
		return QOBJ_ERROR_INVALID_FILE;

	FILE* fptr;
	if(fopen_s(&fptr, path, "r") != 0)
		return QOBJ_ERROR_IO;

	//allocate memory:
	//---------------
	*materials = (QOBJmaterial*)QOBJ_MALLOC(sizeof(QOBJmaterial));
	*numMaterials = 0;

	if(!*materials)
	{
		QOBJ_FREE(materials);
		return QOBJ_ERROR_OUT_OF_MEM;
	}

	//main loop:
	//---------------
	QOBJerror errorCode = QOBJ_SUCCESS;

	char curToken[QOBJ_MAX_TOKEN_LEN];
	char curTokenEnd;

	uint32_t curMaterial = 0;

	while(1)
	{
		qobj_next_token(fptr, curToken, &curTokenEnd);

		if(curTokenEnd == EOF)
			break;

		if(curToken[0] == '\0')
			continue;

		if(curToken[0] == '#' || strcmp(curToken, "illum") == 0 ||
		   strcmp(curToken, "Tf") == 0) //comments / ignored commands
		{
			if(isspace(curTokenEnd))
				qobj_fgets(fptr, curToken, &curTokenEnd);
		}
		else if(strcmp(curToken, "newmtl") == 0)
		{
			qobj_fgets(fptr, curToken, &curTokenEnd);

			curMaterial = *numMaterials;
			QOBJmaterial* newMaterials = (QOBJmaterial*)QOBJ_REALLOC(*materials, *numMaterials * sizeof(QOBJmaterial));
			if(!newMaterials)
			{
				errorCode = QOBJ_ERROR_OUT_OF_MEM;
				break;
			}
			else
			{
				*materials = newMaterials;
				(*numMaterials)++;
			}

			(*materials)[curMaterial] = qobj_default_material();
			(*materials)[curMaterial].name = (char*)QOBJ_MALLOC(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			strcpy_s((*materials)[curMaterial].name, QOBJ_MAX_TOKEN_LEN, curToken);
		}
		else if(strcmp(curToken, "Ka") == 0)
		{
			QOBJcolor col;
			fscanf_s(fptr, "%f %f %f\n", &col.r, &col.g, &col.b);

			(*materials)[curMaterial].ambientColor = col;
		}
		else if(strcmp(curToken, "Kd") == 0)
		{
			QOBJcolor col;
			fscanf_s(fptr, "%f %f %f\n", &col.r, &col.g, &col.b);

			(*materials)[curMaterial].diffuseColor = col;
		}
		else if(strcmp(curToken, "Ks") == 0)
		{
			QOBJcolor col;
			fscanf_s(fptr, "%f %f %f\n", &col.r, &col.g, &col.b);

			(*materials)[curMaterial].specularColor = col;
		}
		else if(strcmp(curToken, "d") == 0)
		{
			float opacity;
			fscanf_s(fptr, "%f\n", &opacity);

			(*materials)[curMaterial].opacity = opacity;
		}
		else if(strcmp(curToken, "Ns") == 0)
		{
			float specularExp;
			fscanf_s(fptr, "%f\n", &specularExp);

			(*materials)[curMaterial].specularExp = specularExp;
		}
		else if(strcmp(curToken, "Ni") == 0)
		{
			float refractionIndex;
			fscanf_s(fptr, "%f\n", &refractionIndex);

			(*materials)[curMaterial].refractionIndex = refractionIndex;
		}
		else if(strcmp(curToken, "map_Ka") == 0)
		{
			char* mapPath = (char*)QOBJ_MALLOC(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].ambientMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Kd") == 0)
		{
			char* mapPath = (char*)QOBJ_MALLOC(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].diffuseMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Ks") == 0)
		{
			char* mapPath = (char*)QOBJ_MALLOC(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].specularMapPath = mapPath;
		}
		else if(strcmp(curToken, "map_Bump") == 0)
		{
			char* mapPath = (char*)QOBJ_MALLOC(QOBJ_MAX_TOKEN_LEN * sizeof(char));
			qobj_fgets(fptr, mapPath, &curTokenEnd);

			(*materials)[curMaterial].normalMapPath = mapPath;
		}
	}

	//cleanup
	//---------------
	if(errorCode != QOBJ_SUCCESS)
	{
		qobj_free_mtl(*numMaterials, *materials);
		*numMaterials = 0;
	}

	fclose(fptr);
	return errorCode;	
}

void qobj_free_mtl(uint32_t numMaterials, QOBJmaterial* materials)
{
	if(materials == NULL)
		return;

	for(uint32_t i = 0; i < numMaterials; i++)
		qobj_material_free(materials[i]);

	QOBJ_FREE(materials);
}

//----------------------------------------------------------------------//

#endif //#ifdef QOBJ_IMPLEMENTATION

#ifdef __cplusplus
} //extern "C"
#endif

#endif //QOBJ_H