#include "ChunkGenerator.h"

ChunkGenerator::ChunkGenerator()
	:mPlayerPos(0,0,0), mInterChunkPos(0,0,0),mDone(false) {}
//---------------------------------------------------------------------------

ChunkGenerator::~ChunkGenerator() 
{
	//stopThread();
	//mThread.join();
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
	boost::mutex::scoped_lock lock(mPlayerPosMutex);
	mInterChunkPos = InterChunkCoords(
		floor(mPlayerPos.x) / CHUNK_SIZE_X,
		floor(mPlayerPos.y) / CHUNK_SIZE_Y,
		floor(mPlayerPos.z) / CHUNK_SIZE_Z);
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
