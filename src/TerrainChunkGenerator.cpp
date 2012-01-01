#include "TerrainChunkGenerator.h"
#include <noise/noise.h>


//void addTree(byte data[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z], int i, int h, int k);
bool addTree(byte data[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z], int i, int h, int k,
	std::list<ChunkCoords>& changes);

TerrainChunkGenerator::TerrainChunkGenerator()
{
	mPerlin = new noise::module::Perlin();
	//mRidged = new noise::module::RidgedMulti();
	//mBillow = new noise::module::Billow();
	//mPerlin->SetSeed(rand());
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

	// generate (this could be threaded, but doesn't seem too slow just yet)
	for(int p = 0; p < 3; ++p)
	{
		if(!mObserverPositions[p].first)
			continue;

		int j = 0;
		for(int i = -GENERATE_CHUNK_DISTANCE; i <= GENERATE_CHUNK_DISTANCE; ++i)
			for(int k = -GENERATE_CHUNK_DISTANCE; k <= GENERATE_CHUNK_DISTANCE; ++k)
		{
			if(mChunks.find(InterChunkCoords(i,j,k) + mObserverPositions[p].second) == mChunks.end())
			{
				++mNumGeneratedChunks;

				InterChunkCoords loc(i,j,k);
				loc = loc + mObserverPositions[p].second;

				BasicChunk* c = new TerrainChunk(this, loc);

				if(loc.x==0&&loc.y==0&&loc.z==0)
				{
					boost::mutex::scoped_lock lock(mDirtyListMutex);
					mDirtyChunks[c] = true;
				}

				memset(c->blocks, 0, CHUNK_VOLUME);
				memset(c->light, 0, CHUNK_VOLUME);

				for(int x = 0; x < CHUNK_SIZE_X; ++x)
					for(int z = 0; z < CHUNK_SIZE_Z; ++z)
				{
					// hackity hacky terrain gen
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
							c->blocks[x][height][z] = BT_GRASS;
						else if(y > height - 4)
							c->blocks[x][y][z] = BT_DIRT;
						else
							c->blocks[x][y][z] = BT_STONE;
					}

					for(int y = height + 1; y < CHUNK_SIZE_Y; ++y)
					{
						c->light[x][y][z] = 15;
					}

					if(addTree(c->blocks, x, height, z, c->mChanges))
					{
						boost::mutex::scoped_lock lock(mChangeSetMutex);
						mChangedChunks.insert(c);
					}
				}

				mChunks[loc] = c;
				newChunks.push_back(std::make_pair(c, loc));
			}
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

	// loop through all active chunks and deactivate all that don't need it
	for(std::list<BasicChunk*>::iterator it = mActiveChunks.begin(); it != mActiveChunks.end(); ++it)
	{
		InterChunkCoords crds = (*it)->getInterChunkPosition();
		bool shouldRemainActive = false;

		for(int p = 0; p < 3; ++p)
		{
			if(!mObserverPositions[p].first)
				continue;

			if(abs(mObserverPositions[p].second.x - crds.x) <= DEACTIVATE_CHUNK_DISTANCE &&
			   abs(mObserverPositions[p].second.z - crds.z) <= DEACTIVATE_CHUNK_DISTANCE)
			{
				shouldRemainActive = true;
				break;
			}
		}

		if(!shouldRemainActive)
		{
			--mNumActiveChunks;
			(*it)->deactivate();
			boost::mutex::scoped_lock lock(mBuiltListMutex);
			mBuiltChunks[*it] = true;
			it = mActiveChunks.erase(it);
		}
	}

	// now activate chunks around each observer
	for(int p = 0; p < 3; ++p)
	{
		if(!mObserverPositions[p].first)
			continue;

		int j = 0;
		for(int i = -ACTIVE_CHUNK_DISTANCE; i <= ACTIVE_CHUNK_DISTANCE; ++i)
			for(int k = -ACTIVE_CHUNK_DISTANCE; k <= ACTIVE_CHUNK_DISTANCE; ++k)
		{
			std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
				mChunks.find(InterChunkCoords(i,j,k) + mObserverPositions[p].second);

			if(iter != mChunks.end())
			{
				if(!iter->second->isActive())
				{
					++mNumActiveChunks;
					mActiveChunks.push_back(iter->second);

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
	}

	// setup newly active chunks' neighbors for a lighting update
	for(std::list<BasicChunk*>::iterator it = 
		newlyActiveChunks.begin(); it != newlyActiveChunks.end(); ++it)
	{
		int j = 0;
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
			std::map<ChunkCoords, ChunkChange>::iterator iter = bc->mConfirmedChanges.find(*i);

			if(iter == bc->mConfirmedChanges.end())
			{
				byte type = bc->blocks[(*i).x][(*i).y][(*i).z];
				byte newType = (*i).data;
				if(type == newType)
					continue;
				int8 l1 = BLOCKTYPES[type] & BP_EMISSIVE ? BLOCKTYPES[type] & 0x0F : 0;
				int8 l2 = BLOCKTYPES[newType] & BP_EMISSIVE ? BLOCKTYPES[newType] & 0x0F : 0;
				bc->mConfirmedChanges[*i] = ChunkChange(type, l2 - l1);
			}
			else
			{
				// if this cancels the operation out, remove it
				if(iter->second.origBlock == (*i).data)
				{
					i = bc->mChanges.erase(i);
				}
				// otherwise just update
				else
				{
					byte newType = (*i).data;
					int8 l1 = BLOCKTYPES[iter->second.origBlock] & BP_EMISSIVE ? 
						BLOCKTYPES[iter->second.origBlock] & 0x0F : 0;
					int8 l2 = BLOCKTYPES[newType] & BP_EMISSIVE ? BLOCKTYPES[newType] & 0x0F : 0;
					iter->second.newLight = l2 - l1;
					//iter->first.data = newType;
					const ChunkCoords& tmp = iter->first;
					const_cast<ChunkCoords&>(tmp).data = newType;
				}
			}
		}

		for(std::map<ChunkCoords, ChunkChange>::iterator i = bc->mConfirmedChanges.begin(); 
			i != bc->mConfirmedChanges.end(); ++i)
		{
			bc->blocks[i->first.x][i->first.y][i->first.z] = i->first.data;
			needsRebuild = true;
			relight = true;

			if(i->first.x == 0)
				changedNeighbors[0] = true;
			else if(i->first.x == CHUNK_SIZE_X-1)
				changedNeighbors[1] = true;
			if(i->first.z == 0)
				changedNeighbors[4] = true;
			else if(i->first.z == CHUNK_SIZE_Z-1)
				changedNeighbors[5] = true;
		}

		// make sure to clear the changes
		bc->mChanges.clear();

		// mark block and neighbors dirty
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

		// this will actually just means that these chunks get peeked at in the update
		// process, and updated if they've been marked dirty by an actual change
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
bool addTree(byte data[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z], int i, int h, int k,
	std::list<ChunkCoords>& changes)
{
	if(i>2&&i<CHUNK_SIZE_X-3&&
		k>2&&k<CHUNK_SIZE_Z-3&&rand()%150==0)
	{
		changes.push_back(ChunkCoords(i,h+1,k,BT_WOOD));
		changes.push_back(ChunkCoords(i,h+2,k,BT_WOOD));
		changes.push_back(ChunkCoords(i,h+3,k,BT_WOOD));
		changes.push_back(ChunkCoords(i,h+4,k,BT_WOOD));
		changes.push_back(ChunkCoords(i+1,h+4,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+4,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+4,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+4,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+4,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+4,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+4,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+4,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+4,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+4,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+4,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+4,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+4,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+4,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+4,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+4,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+4,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+4,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+4,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+4,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+5,k,BT_WOOD));
		changes.push_back(ChunkCoords(i+1,h+5,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+5,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+5,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+5,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+5,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+5,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+5,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+5,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+5,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+5,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+5,k+2,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+5,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+5,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+5,k-2,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+5,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+5,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i+2,h+5,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+5,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+5,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i-2,h+5,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+6,k,BT_WOOD));
		changes.push_back(ChunkCoords(i+1,h+6,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+6,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+6,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+6,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+6,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+6,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+6,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+6,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i+1,h+7,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+7,k+1,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+7,k-1,BT_LEAVES));
		changes.push_back(ChunkCoords(i-1,h+7,k,BT_LEAVES));
		changes.push_back(ChunkCoords(i,h+7,k,BT_LEAVES));
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------
