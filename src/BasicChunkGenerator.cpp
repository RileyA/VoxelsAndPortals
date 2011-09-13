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

BasicChunkGenerator::~BasicChunkGenerator()
{
	stopThread();
	mThread.join();

	// delete all the chunks...
}

void BasicChunkGenerator::generateThread(BasicChunkGenerator* gen)
{
	// build the thread pool
	for(int i = 0; i < 6; ++i)
	{
		gen->mThreadPool.add_thread(new boost::thread(&workerThread, gen));
	}
	
	while(true)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(30)); 

		{
			boost::mutex::scoped_lock lock(gen->mExitLock);
			if(gen->mDone)
			{
				// flood the job queue with null jobs...
				for(int i = 0; i < 12; ++i)
					gen->addJob(0);
				// start the workers (no workers should be working at this point)
				gen->startWorkers();
				//  let them all wrap up
				gen->mThreadPool.join_all();
				break;
			}
		}

		Real tt = TimeManager::getPtr()->getTimeDecimal();
		gen->generate();

		gen->apply();
	

		gen->light();
		gen->waitForJobs();

		//tt = TimeManager::getPtr()->getTimeDecimal() - tt;
		//if(tt > 0.0001)
		//	std::cout<<"Lighting took: "<<tt<<"\n";

		gen->build();
		gen->waitForJobs();
		//if(tt > 0.0001)
		//	std::cout<<"Updates took: "<<tt<<"\n";
	}
}

void BasicChunkGenerator::update(Real delta)
{
	boost::mutex::scoped_lock lock(mBuiltListMutex);

	for(std::map<BasicChunk*, bool>::iterator it = mBuiltChunks.begin(); it != mBuiltChunks.end(); ++it)
	{
	//	if(it->second)
	//		std::cout<<"Full build\n";
	//	else
	//		std::cout<<"Light build\n";
		//{
			it->first->buildMesh(it->second);
		//}
	}

	mBuiltChunks.clear();
}

void BasicChunkGenerator::startThread()
{
	mThread = boost::thread(&BasicChunkGenerator::generateThread, this);
}

void BasicChunkGenerator::stopThread()
{
	boost::mutex::scoped_lock lock(mExitLock);
	mDone = true;
}

void BasicChunkGenerator::generate()
{
	// generate and activate chunks (also eventually deactivate some of them..)
	Vector3 playerPos;

	// get player position all thread safe-like
	{
		boost::mutex::scoped_lock lock(mPlayerPosMutex);
		playerPos = mPlayerPos;
	}

	// translate player position into interchunk coordinates
	// apply magical offset
	//playerPos += OFFSET; // now there's a constant asking for a conflict...
	
	// floor and divide
	int x = floor(playerPos.x);
	int y = floor(playerPos.y);
	int z = floor(playerPos.z);
	x /= CHUNK_SIZE_X;
	y /= CHUNK_SIZE_Y;
	z /= CHUNK_SIZE_Z;

	std::list<std::pair<BasicChunk*,InterChunkCoords> > newChunks;

	// generate (this could be threaded if I were generating anything expensive)
	for(int i = -2; i <= 2; ++i)
		for(int j = -2; j <= 2; ++j)
			for(int k = -2; k <= 2; ++k)
	{
		if(mChunks.find(InterChunkCoords(x + i, y + j, z + k)) == mChunks.end())
		{
			++numGeneratedChunks;
			// make the new chunk(!1!!1)
			InterChunkCoords loc(x + i, y + j, z + k);
			BasicChunk* c = new BasicChunk(this, loc);

			if(loc.x==0&&loc.y==0&&loc.z==0)
			{
				memset(c->blocks, 0, CHUNK_VOLUME);
				boost::mutex::scoped_lock lock(mDirtyListMutex);
				mDirtyChunks[c] = true;

				byte xx = rand()%CHUNK_SIZE_X;
				byte yy = rand()%CHUNK_SIZE_Y;
				byte zz = rand()%CHUNK_SIZE_Z;
				c->lights.push_back(ChunkCoords(xx,yy,zz, 15));
				//c->blocks[xx][yy][zz] = 6;


				for(int f = 0; f < 20; ++f) 
				{
					xx = rand()%CHUNK_SIZE_X;
					yy = rand()%CHUNK_SIZE_Y;
					zz = rand()%CHUNK_SIZE_Z;
					c->lights.push_back(ChunkCoords(xx,yy,zz, 15));
					//c->blocks[xx][yy][zz] = 6;
				}
			}
			else
			{
				memset(c->blocks, 2, CHUNK_VOLUME);
				//c->blocks[8][8][8] = 0;
				//c->blocks[8][9][8] = 0;

				/*for(int l = 0; l < CHUNK_SIZE_X; ++l)
					for(int m = 0; m < CHUNK_SIZE_Z; ++m)
				{
					c->blocks[l][rand()%14+2][m] = rand()%6;
				}*/
			}

				
			memset(c->light, 0, CHUNK_VOLUME);
			mChunks[loc] = c;
			newChunks.push_back(std::make_pair(c, loc));
		}
	}

	//if(newChunks.size())
	//	std::cout<<"Generated "<<newChunks.size()<<" new chunks!\n";

	// neighborify(TM) the new chunks and their neighbors
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

	std::list<BasicChunk*> newlyActiveChunks;

	// activate
	for(int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j)
			for(int k = -1; k <= 1; ++k)
	{
		std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
			mChunks.find(InterChunkCoords(x + i, y + j, z + k));
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

	// setup newly active chunks' neighbors for lighting update
	for(std::list<BasicChunk*>::iterator it = 
		newlyActiveChunks.begin(); it != newlyActiveChunks.end(); ++it)
	{
		for(int i = -1; i <= 1; ++i)
			for(int j = -1; j <= 1; ++j)
				for(int k = -1; k <= 1; ++k)
		{
			std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
				mChunks.find(InterChunkCoords(i + (*it)->position.x, j + (*it)->position.x, k + (*it)->position.x));
			if(iter != mChunks.end())
			{
				std::map<BasicChunk*, bool>::iterator itera = 
					mDirtyChunks.find(iter->second);
				if(itera == mDirtyChunks.end())
				{
					mDirtyChunks[iter->second] = false;
				}
			}
		}
	}
	
}

void BasicChunkGenerator::apply()
{
	// apply changes to chunks (only time chunk block data will ever change outside of creation)
	boost::mutex::scoped_lock lock(mChangeSetMutex);

	for(std::set<BasicChunk*>::iterator it = mChangedChunks.begin(); it != mChangedChunks.end(); ++it)
	{
		BasicChunk* bc = (*it);
		boost::mutex::scoped_lock lock(bc->mBlockMutex);

		bool needsRebuild = false;
		bool relight = true;

		std::list<BasicChunk*> changedNeighbors;

		for(std::list<ChunkCoords>::iterator i = bc->mChanges.begin(); i != bc->mChanges.end(); ++i)
		{
			if(bc->blocks[(*i).c.x][(*i).c.y][(*i).c.z] != (*i).c.data)
			{
				bc->blocks[(*i).c.x][(*i).c.y][(*i).c.z] = (*i).c.data;
				needsRebuild = true;

				// no need to redo lighting in some cases, that I will test later...
				//if(bc->getLightAt((*i)) == 0)
				//{
				//	relight = false;
				//}

				if(i->onEdge())
				{
					if(i->c.x == 0)
						changedNeighbors.push_back(bc->neighbors[0]);
					else if(i->c.x == CHUNK_SIZE_Y-1)
						changedNeighbors.push_back(bc->neighbors[1]);
					if(i->c.y == 0)
						changedNeighbors.push_back(bc->neighbors[2]);
					else if(i->c.y == CHUNK_SIZE_Y-1)
						changedNeighbors.push_back(bc->neighbors[3]);
					if(i->c.z == 0)
						changedNeighbors.push_back(bc->neighbors[4]);
					else if(i->c.z == CHUNK_SIZE_Z-1)
						changedNeighbors.push_back(bc->neighbors[5]);
				}
			}
		}

		bc->mChanges.clear();

		if(needsRebuild)//(*it)->applyChanges())
		{
			boost::mutex::scoped_lock lock(mDirtyListMutex);

			for(std::list<BasicChunk*>::iterator cn = changedNeighbors.begin(); cn != changedNeighbors.end();
				++cn)
			{
				mDirtyChunks[*cn] = true;
			}

			mDirtyChunks[*it] = true;
		}

		if(needsRebuild || relight)
		{
			// relight surrounding blocks
			for(int i = -1; i <= 1; ++i)
				for(int j = -1; j <= 1; ++j)
					for(int k = -1; k <= 1; ++k)
			{
				std::map<InterChunkCoords,BasicChunk*>::iterator iter = 
					mChunks.find(InterChunkCoords(bc->position.x + i, 
						bc->position.y + j, bc->position.z + k));
				if(iter != mChunks.end())
				{
					std::map<BasicChunk*, bool>::iterator itera = 
						mDirtyChunks.find(iter->second);
					if(itera == mDirtyChunks.end())
					{
						mDirtyChunks[iter->second] = false;
					}
				}
			}
		}
	}
	
	mChangedChunks.clear();
}

void BasicChunkGenerator::light()
{
	// do lighting calculations where needed
	std::map<BasicChunk*, bool>::iterator it = mDirtyChunks.begin();

	// all dirty chunks need relighting (probably, optimization can come later...)
	for(it; it != mDirtyChunks.end(); ++it)
	{
		it->first->clearLighting();
		addJob(new LightJob(mDirtyChunks, this, it->first, it->second));
	}

	startWorkers();
}

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

	// start up the threads
	startWorkers();
}

void BasicChunkGenerator::workerThread(BasicChunkGenerator* gen)
{
	Job* assigned = 0;
	bool moreJobs = false;
	bool hasJob = false; // poor unemployed worker thread.. ;_;

	// the only way it will terminate is if it receives a null job
	while(true)
	{

		{
			boost::mutex::scoped_lock lock(gen->mJobMutex);

			// skip if this is the first iteration
			if(hasJob)
			{
				// must've just wrapped up a job
				--gen->mActiveJobs;
				--gen->mPendingJobs;

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
			hasJob = true;
			++gen->mActiveJobs;
			assigned = gen->mJobs.front();
			gen->mJobs.pop_front();

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

void BasicChunkGenerator::notifyChunkChange(BasicChunk* c)
{
	boost::mutex::scoped_lock lock(mChangeSetMutex);
	mChangedChunks.insert(c);
}
