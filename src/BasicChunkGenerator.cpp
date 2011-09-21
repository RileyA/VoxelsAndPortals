#include "Common.h"
#include "BasicChunkGenerator.h"

BasicChunkGenerator::BasicChunkGenerator()
{
	mDone = false;
	mNumGeneratedChunks = 0;
	mNumActiveChunks = 0;
	mThreadPool = 0;
}
//---------------------------------------------------------------------------

BasicChunkGenerator::~BasicChunkGenerator()
{
	stopThread();
	mThread.join();

	// delete all the chunks...
	for(std::map<InterChunkCoords, BasicChunk*>::iterator i = mChunks.begin();
		i != mChunks.end(); ++i)
	{
		delete i->second;
	}

	mChunks.clear();
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::backgroundThread()
{
	// make thread pool
	mThreadPool = new ThreadPool(6);
	
	while(true)
	{
		// Check if it's been signalled to exit
		{
			boost::mutex::scoped_lock lock(mExitLock);

			if(mDone)
			{
				delete mThreadPool;
				mThreadPool = 0;
				break;
			}
		}

		// update player interchunk coords
		updatePlayerPos();

		// generate and activate
		generate();

		// manage activation/deactivation of chunks
		activate();

		// apply any changes to chunks
		apply();

		// build chunks that need it before lighting (lighting is easily the most expensive operation, so
		// building first allows changes to take effect quicker (but causes awkward light changes...)
		/*{
			boost::mutex::scoped_lock lock(mDirtyListMutex);
			
			std::map<BasicChunk*, bool>::iterator it = mDirtyChunks.begin();

			for(it; it != mDirtyChunks.end(); ++it)
			{
				if(it->second)
					mThreadPool->addJob(new BuildJob(it->first, this, it->second));
				it->second = false;
			}
		}
		mThreadPool->startWorkers();
		mThreadPool->waitForWorkers();*/
	
		// do lighting calculations (using thread pool)
		light();
		mThreadPool->startWorkers();
		mThreadPool->waitForWorkers();

		// build meshes (using thread pool)
		build();
		mThreadPool->startWorkers();
		mThreadPool->waitForWorkers();
	}
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::update(Real delta)
{
	boost::mutex::scoped_lock lock(mBuiltListMutex);

	// update all meshes that have been built
	for(std::map<BasicChunk*, bool>::iterator it = mBuiltChunks.begin(); 
		it != mBuiltChunks.end(); ++it)
	{
		it->first->update(it->second);
	}

	mBuiltChunks.clear();
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::notifyChunkLightChange(BasicChunk* c)
{
	boost::mutex::scoped_lock lock(mDirtyListMutex);

	std::map<BasicChunk*, bool>::iterator look = 
		mDirtyChunks.find(c);

	if(look == mDirtyChunks.end())
	{
		mDirtyChunks[c] = false;

		for(int i = -1; i <= 1; ++i)
			for(int j = -1; j <= 1; ++j)
				for(int k = -1; k <= 1; ++k)
		{
			InterChunkCoords temp = c->getInterChunkPosition() + InterChunkCoords(i,j,k);
			
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

void BasicChunkGenerator::generate()
{
	// keep a list of newly generated chunks
	std::list<std::pair<BasicChunk*,InterChunkCoords> > newChunks;

	// generate (this could be threaded if it were generating anything expensive)
	for(int i = -2; i <= 2; ++i)
		for(int j = -2; j <= 2; ++j)
			for(int k = -2; k <= 2; ++k)
	{
		if(mChunks.find(InterChunkCoords(i,j,k) + mInterChunkPos) == mChunks.end())
		{
			++mNumGeneratedChunks;

			InterChunkCoords loc(i,j,k);
			loc = loc + mInterChunkPos;

			BasicChunk* c = new BasicChunk(this, loc);

			if(loc.x==0&&loc.y==0&&loc.z==0)
			{
				memset(c->blocks, 0, CHUNK_VOLUME);
				boost::mutex::scoped_lock lock(mDirtyListMutex);
				mDirtyChunks[c] = true;

				byte x = rand()%CHUNK_SIZE_X;
				byte y = rand()%CHUNK_SIZE_Y;
				byte z = rand()%CHUNK_SIZE_Z;
				c->lights.push_back(ChunkCoords(x,y,z, 15));

				for(int f = 0; f < 20; ++f) 
				{
					x = rand()%CHUNK_SIZE_X;
					y = rand()%CHUNK_SIZE_Y;
					z = rand()%CHUNK_SIZE_Z;
					c->lights.push_back(ChunkCoords(x,y,z, 15));
				}
			}
			else
			{
				memset(c->blocks, 2, CHUNK_VOLUME);
			}

			memset(c->light, 0, CHUNK_VOLUME);
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

void BasicChunkGenerator::activate()
{
	// keep a list of chunks that have been activated
	std::list<BasicChunk*> newlyActiveChunks;

	// activate all chunks within a 3x3 cube around the player
	for(int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j)
			for(int k = -1; k <= 1; ++k)
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

	// setup newly active chunks' neighbors for a lighting update
	for(std::list<BasicChunk*>::iterator it = 
		newlyActiveChunks.begin(); it != newlyActiveChunks.end(); ++it)
	{
		for(int i = -1; i <= 1; ++i)
			for(int j = -1; j <= 1; ++j)
				for(int k = -1; k <= 1; ++k)
		{
			InterChunkCoords temp = (*it)->getInterChunkPosition() + InterChunkCoords(i,j,k);
			
			std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
				mChunks.find(temp);

			// if the neighbor exists, mark it for light update
			if(iter != mChunks.end())
			{
				boost::mutex::scoped_lock lock(mDirtyListMutex);

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

void BasicChunkGenerator::apply()
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
				if(i->onEdge())
				{
					if(i->x == 0)
						changedNeighbors[0] = true;
					else if(i->x == CHUNK_SIZE_Y-1)
						changedNeighbors[1] = true;
					if(i->y == 0)
						changedNeighbors[2] = true;
					else if(i->y == CHUNK_SIZE_Y-1)
						changedNeighbors[3] = true;
					if(i->z == 0)
						changedNeighbors[4] = true;
					else if(i->z == CHUNK_SIZE_Z-1)
						changedNeighbors[5] = true;
				}
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
			for(int i = -1; i <= 1; ++i)
				for(int j = -1; j <= 1; ++j)
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

void BasicChunkGenerator::light()
{
	// do lighting calculations where needed
	std::map<BasicChunk*, bool>::iterator it = mDirtyChunks.begin();

	// all dirty chunks need relighting (probably, optimization can come later...)
	for(it; it != mDirtyChunks.end(); ++it)
	{
		// we have to clear lighting first
		it->first->clearLighting();
		mThreadPool->addJob(new LightJob(mDirtyChunks, this, it->first, it->second));
	}
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::build()
{	
	// build all dirty chunks
	boost::mutex::scoped_lock lock(mDirtyListMutex);
	
	std::map<BasicChunk*, bool>::iterator it = mDirtyChunks.begin();

	for(it; it != mDirtyChunks.end(); ++it)
	{
		mThreadPool->addJob(new BuildJob(it->first, this, it->second));
	}

	// all dirty chunks are now pending rebuilds, so we can just clear the whole thing
	mDirtyChunks.clear();
}
//---------------------------------------------------------------------------
