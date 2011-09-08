#ifndef Chunk_H
#define Chunk_H

#include "Oryx.h"
#include "Oryx3DMath.h"
#include "ChunkCoords.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

class Chunk
{
public:

	/** Lock the mesh data for use (namely for creating a mesh...) 
	 *		@param flags In the case of a partial update (lighting only, for instance)
	 *			this will contain info on what is present in this update  */
	MeshData* lockMeshData(unsigned int& flags);

	/** Unlocks the mesh data (this also deletes it, so ensure you lock it 
	 *		once and do everything you need with it)  */
	void unlockMeshData();

	/** Adds a change to this Chunk's change list (it will be updated by the background thread
	 *	at some point) */
	void addChange(ChunkCoords c, byte changeTo);

	/** Sets whether or not this should be active (aka, visible, present in the physics sim, etc) */
	void setActive(bool active);

	/** Gets of this chunk is active */
	bool isActive();

	/** Actual data (public, but shouldn't be accessed directly in most cases...) */
	byte blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
	byte light[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

protected:

	std::set<ChunkCoords> mChanges;
	ChunkManager* mCreator;

	// Whether or not this chunk is 'active' (i.e. visible)
	bool mActive;

	// A temporary mesh data struct, used when the mesh is updated
	MeshData* mData;

	// Pending changes to the chunk
	std::list<ChunkCoords> mChanges;

	// main mutex
	boost::try_mutex mMutex;
	
	// separate mutex for adding changes to the changelist (since they can be added
	// even when the chunk is being updated)
	boost::try_mutex mChangeMutex;

};

#endif
