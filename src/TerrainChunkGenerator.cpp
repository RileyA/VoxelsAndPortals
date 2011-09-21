#include "TerrainChunkGenerator.h"
#include <noise/noise.h>

void addTree(byte data[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z], int i, int h, int k);

TerrainChunkGenerator::TerrainChunkGenerator()
{
	mPerlin = new noise::module::Perlin();
}
//---------------------------------------------------------------------------

TerrainChunkGenerator::~TerrainChunkGenerator()
{
	delete mPerlin;
}
//---------------------------------------------------------------------------

void TerrainChunkGenerator::generate()
{
	// keep a list of newly generated chunks
	std::list<std::pair<BasicChunk*,InterChunkCoords> > newChunks;

	// generate (this could be threaded if it were generating anything expensive)
	int j = 0;
	for(int i = -5; i <= 5; ++i)
		for(int k = -5; k <= 5; ++k)
	{
		if(mChunks.find(InterChunkCoords(i,j,k) + mInterChunkPos) == mChunks.end())
		{
			++mNumGeneratedChunks;

			InterChunkCoords loc(i,j,k);
			loc = loc + mInterChunkPos;

			BasicChunk* c = new TerrainChunk(this, loc);

			if(loc.x==0&&loc.y==0&&loc.z==0)
			{
				boost::mutex::scoped_lock lock(mDirtyListMutex);
				mDirtyChunks[c] = true;
			}

			memset(c->blocks, 0, CHUNK_VOLUME);
			memset(c->light, 9, CHUNK_VOLUME);

			for(int x = 0; x < CHUNK_SIZE_X; ++x)
				for(int z = 0; z < CHUNK_SIZE_Z; ++z)
			{
				// hackity hack
				int height = 50 + mPerlin->GetValue(
					static_cast<double>(x + loc.x * CHUNK_SIZE_X) * 0.0035,
					static_cast<double>(0) * 0.0035,
					static_cast<double>(z + loc.z * CHUNK_SIZE_Z) * 0.0035) * 25
				+ mPerlin->GetValue(
					static_cast<double>(x + loc.x * CHUNK_SIZE_X) * 0.007,
					static_cast<double>(4000) * 0.007,
					static_cast<double>(z + loc.z * CHUNK_SIZE_Z) * 0.007) * 
				mPerlin->GetValue(
					static_cast<double>(x + loc.x * CHUNK_SIZE_X) * 0.017,
					static_cast<double>(2000) * 0.017,
					static_cast<double>(z + loc.z * CHUNK_SIZE_Z) * 0.017) * 20;

				for(int y = height; y >= 0; --y)
				{
					if(y == height)
						c->blocks[x][height][z] = 4;
					else if(y > height - 4)
						c->blocks[x][y][z] = 3;
					else
						c->blocks[x][y][z] = 2;
				}

				addTree(c->blocks, x, height, z);
			}

			mChunks[loc] = c;
			newChunks.push_back(std::make_pair(c, loc));
		}
	}

	// hook all new chunks up with their neighbors (and vice versa)
	for(std::list<std::pair<BasicChunk*, InterChunkCoords> >::iterator it = 
		newChunks.begin(); it != newChunks.end(); ++it)
	{	
		BasicChunk* bc = it->first;
		for(int p = 0; p < 6; ++p)
		{
			std::map<InterChunkCoords, BasicChunk*>::iterator n = mChunks.find((it->second)<<p);
			if(n != mChunks.end())
			{
				bc->neighbors[p] = n->second;
				n->second->neighbors[AXIS_INVERT[p]] = bc;
			}
		}
	}
}
//---------------------------------------------------------------------------

void TerrainChunkGenerator::activate()
{
	// keep a list of chunks that have been activated
	std::list<BasicChunk*> newlyActiveChunks;

	// activate all chunks within a 5x5 cube around the player
	int j = 0;
	for(int i = -4; i <= 4; ++i)
		for(int k = -4; k <= 4; ++k)
	{
		std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
			mChunks.find(InterChunkCoords(i,j,k) + mInterChunkPos);
		if(iter != mChunks.end())
		{
			if(!iter->second->isActive())
			{
				++mNumActiveChunks;

				if(iter->second->activate())
				{
					boost::mutex::scoped_lock lock(mDirtyListMutex);
					mDirtyChunks[iter->second] = true;
					newlyActiveChunks.push_back(iter->second);
				}
				else
				{
					boost::mutex::scoped_lock lock(mBuiltListMutex);
					mBuiltChunks[iter->second] = true;
				}
			}
		}
	}

	// deactivate chunks outside that radius...
	for(int i = -5; i <= 5; ++i)
		for(int k = -5; k <= 5; ++k)
	{
		if((i > -5 && i < 5) && (k > -5 && k < 5))
			continue;

		std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
			mChunks.find(InterChunkCoords(i,j,k) + mInterChunkPos);
		if(iter != mChunks.end())
		{
			if(iter->second->isActive())
			{
				--mNumActiveChunks;
				iter->second->deactivate();
				boost::mutex::scoped_lock lock(mBuiltListMutex);
				mBuiltChunks[iter->second] = true;
			}
		}
	}

	// setup newly active chunks' neighbors for a lighting update
	for(std::list<BasicChunk*>::iterator it = 
		newlyActiveChunks.begin(); it != newlyActiveChunks.end(); ++it)
	{
		//int j = 0;
		for(int i = -1; i <= 1; ++i)
			for(int k = -1; k <= 1; ++k)
		{
			InterChunkCoords temp = (*it)->getInterChunkPosition() + InterChunkCoords(i,j,k);
			
			std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
				mChunks.find(temp);

			// if the neighbor exists, mark it for light update
			if(iter != mChunks.end())
			{
				// look for it in the dirty chunk list...
				std::map<BasicChunk*, bool>::iterator itera = 
					mDirtyChunks.find(iter->second);

				// if it's already slated for an update, leave it alone, since
				// either it's already setup for lighting, or it needs a full build
				if(itera == mDirtyChunks.end())
					mDirtyChunks[iter->second] = false;
			}
		}
	}
}
//---------------------------------------------------------------------------

void TerrainChunkGenerator::apply()
{
	// apply changes to chunks (only time chunk block data will ever change outside of creation)
	boost::mutex::scoped_lock lock(mChangeSetMutex);

	// loop through all chunks that have reported changes
	for(std::set<Chunk*>::iterator it = mChangedChunks.begin(); it != mChangedChunks.end(); ++it)
	{
		BasicChunk* bc = static_cast<BasicChunk*>(*it);

		// lock this chunk's mutex
		boost::mutex::scoped_lock lock(bc->mBlockMutex);

		// it may be the case that a chunk has updates that don't actually change anything
		bool needsRebuild = false;

		// in some cases recalculating lighting is unneeded
		bool relight = false;

		// keep track of neighbors that need updates
		//std::list<BasicChunk*> changedNeighbors;
		bool changedNeighbors[6] = {0,0,0,0,0,0};

		// TODO: account for updates that cancel each other out
		for(std::list<ChunkCoords>::iterator i = bc->mChanges.begin(); i != bc->mChanges.end(); ++i)
		{
			if(bc->blocks[(*i).x][(*i).y][(*i).z] != (*i).data)
			{
				bc->blocks[(*i).x][(*i).y][(*i).z] = (*i).data;
				needsRebuild = true;

				// TODO test out lighting stuff (I think it'll entail checking surrounding blocks,
				// and if all are completely black, then no lighting update is needed...)
				relight = true;

				// if on edge, a neighbor will need update
				// TODO: optimize out any cases where this may not be needed...
				//if(i->onEdge())
				//{
					if(i->x == 0)
						changedNeighbors[0] = true;
					else if(i->x == CHUNK_SIZE_X-1)
						changedNeighbors[1] = true;
					if(i->z == 0)
						changedNeighbors[4] = true;
					else if(i->z == CHUNK_SIZE_Z-1)
						changedNeighbors[5] = true;
				//}
			}
		}

		// make sure to clear the changes
		bc->mChanges.clear();

		// makr block and neighbors dirty
		if(needsRebuild)
		{
			boost::mutex::scoped_lock lock(mDirtyListMutex);

			for(int i = 0; i < 6; ++i)
			{
				if(changedNeighbors[i])
					mDirtyChunks[bc->neighbors[i]] = true;
			}

			mDirtyChunks[bc] = true;
		}

		if(needsRebuild || relight)
		{
			// relight surrounding blocks
			int j = 0;
			for(int i = -1; i <= 1; ++i)
				for(int k = -1; k <= 1; ++k)
			{
				InterChunkCoords temp = bc->getInterChunkPosition() 
					+ InterChunkCoords(i,j,k);
				std::map<InterChunkCoords,BasicChunk*>::iterator iter =
					mChunks.find(temp);

				if(iter != mChunks.end())
				{
					std::map<BasicChunk*, bool>::iterator itera = 
						mDirtyChunks.find(iter->second);

					// only set if it isn't already there
					if(itera == mDirtyChunks.end())
						mDirtyChunks[iter->second] = false;
				}
			}
		}
	}
	
	// make sure to clear out the list
	mChangedChunks.clear();
}
//---------------------------------------------------------------------------

// hacky little tree addition thingy
void addTree(byte data[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z], int i, int h, int k)
{
	if(i>2&&i<CHUNK_SIZE_X-3&&
		k>2&&k<CHUNK_SIZE_Z-3&&rand()%150==0)
	{
		data[i][h+1][k] = 5;
		data[i][h+2][k] = 5;
		data[i][h+3][k] = 5;
		data[i][h+4][k] = 5;
		data[i+1][h+4][k] = 1;
		data[i][h+4][k+1] = 1;
		data[i][h+4][k-1] = 1;
		data[i-1][h+4][k] = 1;
		data[i+1][h+4][k+1] = 1;
		data[i-1][h+4][k-1] = 1;
		data[i+1][h+4][k-1] = 1;
		data[i-1][h+4][k+1] = 1;
		data[i+1][h+4][k+2] = 1;
		data[i][h+4][k+2] = 1;
		data[i-1][h+4][k+2] = 1;
		data[i+1][h+4][k-2] = 1;
		data[i][h+4][k-2] = 1;
		data[i-1][h+4][k-2] = 1;
		data[i+2][h+4][k+1] = 1;
		data[i+2][h+4][k] = 1;
		data[i+2][h+4][k-1] = 1;
		data[i-2][h+4][k-1] = 1;
		data[i-2][h+4][k] = 1;
		data[i-2][h+4][k+1] = 1;
		data[i][h+5][k] = 5;
		data[i+1][h+5][k] = 1;
		data[i][h+5][k+1] = 1;
		data[i][h+5][k-1] = 1;
		data[i-1][h+5][k] = 1;
		data[i+1][h+5][k+1] = 1;
		data[i-1][h+5][k-1] = 1;
		data[i+1][h+5][k-1] = 1;
		data[i-1][h+5][k+1] = 1;
		data[i+1][h+5][k+2] = 1;
		data[i][h+5][k+2] = 1;
		data[i-1][h+5][k+2] = 1;
		data[i+1][h+5][k-2] = 1;
		data[i][h+5][k-2] = 1;
		data[i-1][h+5][k-2] = 1;
		data[i+2][h+5][k+1] = 1;
		data[i+2][h+5][k] = 1;
		data[i+2][h+5][k-1] = 1;
		data[i-2][h+5][k-1] = 1;
		data[i-2][h+5][k] = 1;
		data[i-2][h+5][k+1] = 1;
		data[i][h+6][k] = 5;
		data[i+1][h+6][k] = 1;
		data[i][h+6][k+1] = 1;
		data[i][h+6][k-1] = 1;
		data[i-1][h+6][k] = 1;
		data[i+1][h+6][k+1] = 1;
		data[i-1][h+6][k-1] = 1;
		data[i+1][h+6][k-1] = 1;
		data[i-1][h+6][k+1] = 1;
		data[i+1][h+7][k] = 1;
		data[i][h+7][k+1] = 1;
		data[i][h+7][k-1] = 1;
		data[i-1][h+7][k] = 1;
		data[i][h+7][k] = 1;
	}
}
//-----------------------------------------------------------------------
