#ifndef ChunkGenerator_H
#define ChunkGenerator_H

#include "Common.h"
#include "ChunkCoords.h"
#include "Chunk.h"

/** Abstract Chunk generator for procedurally creating chunks */
class ChunkGenerator
{
public:

	ChunkGenerator();
	virtual ~ChunkGenerator();

	/** Called from the main thread, does updating of meshes and so forth */
	virtual void update(Real delta) = 0;

	virtual void startThread() = 0;

protected:

};

#endif
