#include <stdio.h>
#include "quickobj.h"

int main()
{
	size_t numMeshes;
	QOBJmesh* meshes;

	if(qobj_load("viking_room.obj", &numMeshes, &meshes) != QOBJ_SUCCESS)
		return -1;

	qobj_free(numMeshes, meshes);
	return 0;
}