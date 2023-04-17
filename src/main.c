#include <stdio.h>
#include "quickobj.h"

int main()
{
	size_t numMeshes;
	QOBJmesh* meshes;
	size_t numMaterials;
	QOBJmaterial* materials;

	if(qobj_load("test/viking_room.obj", &numMeshes, &meshes, &numMaterials, &materials) != QOBJ_SUCCESS)
		return -1;

	qobj_free(numMeshes, meshes, numMaterials, materials);
	return 0;
}