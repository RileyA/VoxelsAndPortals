#include "ChunkGenerator.h"
#include "TerrainChunk.h"

ChunkGenerator::ChunkGenerator()
	:mPlayerPos(0,0,0), mInterChunkPos(0,0,0),mDone(false) 
{
	mPortalsEnabled = false;
	mObserverPositions[0].first = false;
	mObserverPositions[1].first = false;
	mObserverPositions[2].first = false;
}
//---------------------------------------------------------------------------

ChunkGenerator::~ChunkGenerator() 
{
}
//---------------------------------------------------------------------------

void ChunkGenerator::startThread()
{
	mDone = false;
	mThread = boost::thread(&ChunkGenerator::runBackgroundThread, this);
}
//---------------------------------------------------------------------------

void ChunkGenerator::stopThread()
{
	// just set a flag that the running thread will notice upon the start
	// of its next loop (any pending calculations must finish before it will exit)
	boost::mutex::scoped_lock lock(mExitLock);
	mDone = true;
}
//---------------------------------------------------------------------------
	
void ChunkGenerator::runBackgroundThread(ChunkGenerator* gen)
{
	gen->backgroundThread();
}
//---------------------------------------------------------------------------

void ChunkGenerator::updatePlayerPos()
{
	{
		boost::mutex::scoped_lock lock(mPlayerPosMutex);
		mInterChunkPos = InterChunkCoords(
			floor(mPlayerPos.x) / CHUNK_SIZE_X,
			floor(mPlayerPos.y) / CHUNK_SIZE_Y,
			floor(mPlayerPos.z) / CHUNK_SIZE_Z);
		mObserverPositions[0] = std::make_pair(true, mInterChunkPos);
		mObserverPositions[0].second.y = 0;
	}
	{
		boost::mutex::scoped_lock lock(mPortalMutex);

		for(int i = 0; i < 2; ++i)
		{
			bool coordsChanged = !(mPortalData[i].coords[0] == mPortalData[i].prevCoords[0]
				&&  mPortalData[i].coords[1] == mPortalData[i].prevCoords[1]);
			bool wasActive = mPortalData[i].wasActive;
			bool isActive = mPortalData[i].active;

			// no change at all
			if((!wasActive && !isActive)
				|| (mPortalData[i].wasActive && mPortalData[i].active && !coordsChanged))
			{
				mPortalData[i].status = PortalInfo::PS_UNCHANGED;
			}
			else if(wasActive && !isActive)
			{
				mPortalData[i].status = PortalInfo::PS_DEACTIVATED;
			}
			else if(!wasActive && isActive)
			{
				mPortalData[i].status = PortalInfo::PS_ACTIVATED;
				mObserverPositions[i+1].second = mPortalData[i].chunks[1]->getInterChunkPosition();
			}
			else
			{
				mPortalData[i].status = PortalInfo::PS_MOVED;
				mObserverPositions[i+1].second = mPortalData[i].chunks[1]->getInterChunkPosition();
			}

			mObserverPositions[i+1].first = mPortalData[i].active;	
			mObserverPositions[i+1].second.y = 0;
		}
	}

}
//---------------------------------------------------------------------------

void ChunkGenerator::setPlayerPos(Vector3 pos)
{
	boost::mutex::scoped_lock lock(mPlayerPosMutex);
	mPlayerPos = pos;
}
//---------------------------------------------------------------------------

void ChunkGenerator::notifyChunkChange(Chunk* c)
{
	boost::mutex::scoped_lock lock(mChangeSetMutex);
	mChangedChunks.insert(c);
}
//---------------------------------------------------------------------------

void ChunkGenerator::setPortalInfo() 
{
	boost::mutex::scoped_lock lock(mPortalMutex);
	mPortalsEnabled = false;
}
//---------------------------------------------------------------------------

void ChunkGenerator::setPortalInfo(Chunk** chunks, ChunkCoords* coords)
{
	boost::mutex::scoped_lock lock(mPortalMutex);

	mPortalData[0].active = chunks[0] ? true : false;
	mPortalData[1].active = chunks[2] ? true : false;

	if(chunks[0])
		mPortalData[0].chunks[0] = static_cast<TerrainChunk*>(chunks[0]);
	if(chunks[1])
		mPortalData[0].chunks[1] = static_cast<TerrainChunk*>(chunks[1]);
	if(chunks[2])
		mPortalData[1].chunks[0] = static_cast<TerrainChunk*>(chunks[2]);
	if(chunks[3])
		mPortalData[1].chunks[1] = static_cast<TerrainChunk*>(chunks[3]);

	mPortalData[0].coords[0] = coords[0];
	mPortalData[0].coords[1] = coords[1];
	mPortalData[1].coords[0] = coords[2];
	mPortalData[1].coords[1] = coords[3];

	mPortalsEnabled = mPortalData[0].active && mPortalData[1].active;
}
//---------------------------------------------------------------------------
