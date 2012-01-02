#ifndef ChunkGenerator_H
#define ChunkGenerator_H

#include "Common.h"
#include "ChunkCoords.h"
#include "Chunk.h"

class TerrainChunk;

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
	//int mPortalStatus[2];
	bool mPortalsEnabled;
	//std::pair<Chunk*, ChunkCoords> mPortals[2][2];

	struct PortalInfo
	{
		enum PortalStatus
		{
			PS_UNCHANGED,
			PS_ACTIVATED,
			PS_DEACTIVATED,
			PS_MOVED
		};

		PortalInfo()
		{
			active = false;
			wasActive = false;
			status = PS_UNCHANGED;
			for(int i = 0; i < 2; ++i)
			{
				coords[i] = ChunkCoords(0,0,0,0);
				prevCoords[i] = ChunkCoords(0,0,0,0);
				chunks[i] = 0;
				prevChunks[i] = 0;
				light[i] = 0;
			}
		}

		bool active;
		ChunkCoords coords[2];
		TerrainChunk* chunks[2];
		byte light[2];

		ChunkCoords prevCoords[2];
		TerrainChunk* prevChunks[2];
		//byte prevLight[2];
		bool wasActive;
		PortalStatus status;
	};

	PortalInfo mPortalData[2];

	// positions of observers (0 = player, 1,2 = portals)
	std::pair<bool, InterChunkCoords> mObserverPositions[3];
};

#endif
