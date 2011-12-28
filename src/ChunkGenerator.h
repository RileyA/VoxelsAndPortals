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

	/** Start the background thread that will manage the chunk update-age */
	void startThread();

	/** Static wrapper that starts the thread */
	static void runBackgroundThread(ChunkGenerator* gen);
	
	/** The actual update loop (pure virtual) */
	virtual void backgroundThread() = 0;

	/** Stops the background thread */
	void stopThread();

	/** Updates the internal interchunk position of the player */
	void updatePlayerPos();

	/** Called from the main thread to update player position */
	void setPlayerPos(Vector3 pos);

	/** Notify that a chunk has been changed and will need an update */
	void notifyChunkChange(Chunk* c);

	void setPortalInfo();
	void setPortalInfo(Chunk** chunks, ChunkCoords* coords);

protected:

	// Last known player position
	Vector3 mPlayerPos;
	InterChunkCoords mInterChunkPos;
	boost::mutex mPlayerPosMutex;

	// changed chunks
	boost::mutex mChangeSetMutex;
	std::set<Chunk*> mChangedChunks; 

	// background thread stuff
	boost::mutex mExitLock;
	boost::thread mThread;
	bool mDone;

	// portal stuff (hacky)
	boost::mutex mPortalMutex;
	bool mPortalsEnabled;
	std::pair<Chunk*, ChunkCoords> mPortals[2][2];

};

#endif
