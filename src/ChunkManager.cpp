#include "ChunkManager.h"

ChunkManager::ChunkManager()
{
	mGen = 0;
}
//---------------------------------------------------------------------------

ChunkManager::~ChunkManager()
{
	delete mGen;
}
//---------------------------------------------------------------------------

void ChunkManager::init(Vector3 origin, ChunkGenerator* gen)
{
	mGen = gen;
	mGen->startThread();
}
//---------------------------------------------------------------------------

void ChunkManager::update(Real delta)
{
	mGen->update(delta);
}
//---------------------------------------------------------------------------
