#ifndef ChunkManager_H
#define ChunkManager_H

#include "Common.h"
#include "OryxObject.h"
#include "ChunkGenerator.h"

class ChunkManager : public Object
{
public:

	ChunkManager();
	~ChunkManager();

	void init(Vector3 origin, ChunkGenerator* gen);

	/** This is the update function for the main thread (responsible for updating
	 *		Chunk meshes and the like */
	void update(Real delta);

protected:

	ChunkGenerator* mGen;

};

#endif
