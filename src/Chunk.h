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

/** An abstract chunk */
class Chunk
{
public:

	/** Constructor
	 *		@param creator The ChunkGenerator that created this 
	 *		@param position The position in the grid of chunks */
	Chunk(ChunkGenerator* creator, InterChunkCoords position);
	virtual ~Chunk();

	/** Gets the mesh info */
	inline MeshData* getMesh()
	{
		return mMesh;
	}

	/** Gets the real position (center) */
	inline Vector3 getPosition()
	{
		return Vector3(mPosition.x * CHUNK_SIZE_X, 
			mPosition.y * CHUNK_SIZE_Y, mPosition.z * CHUNK_SIZE_Z);
	}

	/** Gets the inter chunk position (coords in the 3d grid of chunks) */
	inline InterChunkCoords getInterChunkPosition()
	{
		return mPosition;
	}

protected:

	// This chunk's position
	InterChunkCoords mPosition;

	// The chunk generator that created this chunk
	ChunkGenerator* mGenerator;

	// Mesh data
	MeshData* mMesh;

	// Graphics object
	Mesh* mGfxMesh;

	// The static physics trimesh
	CollisionObject* mPhysicsMesh;

	// Subsystems for convenience
	OgreSubsystem* mGfx;
	BulletSubsystem* mPhysics;

};

#endif
