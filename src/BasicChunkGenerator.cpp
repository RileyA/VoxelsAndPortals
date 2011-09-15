#include "Common.h"
#include "BasicChunkGenerator.h"

BasicChunkGenerator::BasicChunkGenerator()
{
	mDone = false;
	mActiveJobs = 0;
	mPendingJobs = 0;
	numGeneratedChunks = 0;
	numActiveChunks = 0;
}
//---------------------------------------------------------------------------

BasicChunkGenerator::~BasicChunkGenerator()
{
	stopThread();
	mThread.join();

	// delete all the chunks...
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::backgroundThread()
{
	// build the thread pool
	for(int i = 0; i < 6; ++i)
	{
		mThreadPool.add_thread(new boost::thread(&workerThread, this));
	}
	
	while(true)
	{
		// sleep for a sec, just so this isn't _constantly_ running, esp.
		// when no updates are queued...
		boost::this_thread::sleep(boost::posix_time::milliseconds(30)); 

		// Check if it's been signalled to exit
		{
			boost::mutex::scoped_lock lock(mExitLock);

			if(mDone)
			{
				// flood the job queue with null jobs...
				for(int i = 0; i < 12; ++i)
					addJob(0);

				// start the workers (no workers should be working at this point)
				startWorkers();

				//  let them all wrap up
				mThreadPool.join_all();

				// break outta the loop
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
	
		// do lighting calculations (using thread pool)
		light();
		startWorkers();
		waitForJobs();

		// build meshes (using thread pool)
		build();
		startWorkers();
		waitForJobs();
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
		it->first->buildMesh(it->second);
	}

	mBuiltChunks.clear();
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::generate()
{
	// generate and activate chunks (also eventually deactivate some of them..)

	// keep a list of newly generated chunks
	std::list<std::pair<BasicChunk*,InterChunkCoords> > newChunks;

	// generate (this could be threaded if it were generating anything expensive)
	for(int i = -2; i <= 2; ++i)
		for(int j = -2; j <= 2; ++j)
			for(int k = -2; k <= 2; ++k)
	{
		if(mChunks.find(InterChunkCoords(i,j,k) + mInterChunkPos) == mChunks.end())
		{
			++numGeneratedChunks;

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
			if(iter->second->activate())
			{
				++numActiveChunks;
				boost::mutex::scoped_lock lock(mDirtyListMutex);
				mDirtyChunks[iter->second] = true;
				newlyActiveChunks.push_back(iter->second);
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
			if(bc->blocks[(*i).c.x][(*i).c.y][(*i).c.z] != (*i).c.data)
			{
				bc->blocks[(*i).c.x][(*i).c.y][(*i).c.z] = (*i).c.data;
				needsRebuild = true;

				// TODO test out lighting stuff (I think it'll entail checking surrounding blocks,
				// and if all are completely black, then no lighting update is needed...)
				relight = true;

				// if on edge, a neighbor will need update
				// TODO: optimize out any cases where this may not be needed...
				if(i->onEdge())
				{
					if(i->c.x == 0)
						changedNeighbors[0] = true;
					else if(i->c.x == CHUNK_SIZE_Y-1)
						changedNeighbors[1] = true;
					if(i->c.y == 0)
						changedNeighbors[2] = true;
					else if(i->c.y == CHUNK_SIZE_Y-1)
						changedNeighbors[3] = true;
					if(i->c.z == 0)
						changedNeighbors[4] = true;
					else if(i->c.z == CHUNK_SIZE_Z-1)
						changedNeighbors[5] = true;
				}
			}
		}

		// make sure to clear the changes
		bc->mChanges.clear();

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
	
	// make sure to clear otu the list
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
		addJob(new LightJob(mDirtyChunks, this, it->first, it->second));
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
		addJob(new BuildJob(it->first, this, it->second));
	}

	// all dirty chunks are now pending rebuilds, so we can just clear the whole thing
	mDirtyChunks.clear();
}
//---------------------------------------------------------------------------

void BasicChunkGenerator::workerThread(BasicChunkGenerator* gen)
{
	Job* assigned = 0;
	bool moreJobs = false;
	bool hasJob = false; // poor unemployed worker thread.. ;_;

	// the only way it will terminate is if it receives a null job
	while(true)
	{
		// lock to access job queue
		{
			boost::mutex::scoped_lock lock(gen->mJobMutex);

			// skip if this is the first iteration
			if(hasJob)
			{
				// must've just wrapped up a job
				--gen->mActiveJobs;
				--gen->mPendingJobs;

				// notify if there are no jobs left
				if(gen->mPendingJobs == 0)
					gen->mJobDoneSignal.notify_one();
	
				// if the last job was null, then terminate
				if(!assigned)
					return;
				else
					delete assigned;
			}

			// loop on the condition, to handle spurious wakeups
			// note that this will get skipped if jobs remain in the queue after being awoken
			while(gen->mJobs.empty())
				gen->mJobSignal.wait(lock);

			// if it gets here, there must be a job for it...
			assigned = gen->mJobs.front();
			gen->mJobs.pop_front();
			hasJob = true;
			++gen->mActiveJobs;

			moreJobs = !gen->mJobs.empty();
		}

		// now that the mutex is unlocked, if there's still work to be done, notify another thread
		if(moreJobs)
			gen->mJobSignal.notify_one();

		// actually do the work...
		if(assigned)
			assigned->execute();
	}
}
//---------------------------------------------------------------------------
