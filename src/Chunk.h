#ifndef Chunk_H
#define Chunk_H

#include "Oryx.h"
#include "Oryx3DMath.h"
#include "ChunkCoords.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <set>
#include "ChunkOptions.h"
#include "ChunkCoords.h"

class ChunkManager;
class ChunkGenerator;

class Chunk
{
public:

	Chunk(ChunkGenerator* creator,InterChunkCoords pos);
	virtual ~Chunk();
	//virtual void build() = 0;

	MeshData* getMesh(){return mMesh;}

	Vector3 getPosition()
	{
		return Vector3(position.x * CHUNK_SIZE_X, position.y * CHUNK_SIZE_Y, position.z * CHUNK_SIZE_Z);
	}

	InterChunkCoords position;

protected:

	ChunkGenerator* mGenerator;
	MeshData* mMesh;

	Mesh* mGfxMesh;


};

#endif
