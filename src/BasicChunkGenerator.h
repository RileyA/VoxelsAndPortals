#ifndef BasicChunkGenerator_H
#define BasicChunkGenerator_H

#include "ChunkGenerator.h"
#include "BasicChunk.h"

class BasicChunkGenerator : public ChunkGenerator
{
public:

	BasicChunkGenerator();
	virtual ~BasicChunkGenerator();

	static void generateThread(BasicChunkGenerator* gen);

	virtual void update(Real delta);

	void startThread();

	void stopThread();

	void notifyDirtyChunk(BasicChunk* ch);

	void setPlayerPos(Vector3 pos)
	{
		boost::mutex::scoped_lock lock(mPlayerPosMutex);
		mPlayerPos = pos;
	}

	// these are all to be used in the thread, and there only! (ish, probably)
	void generate();
	void apply();
	void light();
	void build();

	static void lightJob(std::list<BasicChunk*> chunks, std::set<BasicChunk*> allChunks);
	static void buildJob(BasicChunkGenerator* gen, std::list<BasicChunk*> c);

	static void workerThread(BasicChunkGenerator* gen);

	void notifyChunkChange(BasicChunk* c);

	int numGeneratedChunks;
	int numActiveChunks;

private:

	// The chunks themselves
	std::map<InterChunkCoords, BasicChunk*> mChunks;

	// The last known position of the player
	Vector3 mPlayerPos;
	boost::mutex mPlayerPosMutex;

	boost::mutex mExitLock;
	boost::thread mThread;
	bool mDone;

	boost::mutex mChangeSetMutex;
	std::set<BasicChunk*> mChangedChunks; 

	boost::mutex mDirtyListMutex;
	// the bool is whether it needs a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mDirtyChunks; 


	boost::mutex mBuiltListMutex;
	// the bool is whether it needs a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mBuiltChunks; 

	struct Job
	{
		virtual void execute() = 0;
	};

	struct BuildJob : public Job
	{
		BuildJob(BasicChunk* c, BasicChunkGenerator* gen, bool _full)
			:chunk(c),generator(gen),full(_full){}

		void execute()
		{
			// do the build
			chunk->build(full);

			// and notify the build list
			{
				boost::mutex::scoped_lock lock(generator->mBuiltListMutex);

				// in case a full update has been done, but not applied
				if(generator->mBuiltChunks.find(chunk) == generator->mBuiltChunks.end())
					generator->mBuiltChunks[chunk] = full;
			}
		}

		BasicChunk* chunk;
		BasicChunkGenerator* generator;
		bool full;
	};

	struct LightJob : public Job 
	{
		LightJob(const std::map<BasicChunk*, bool>& _chunks, BasicChunkGenerator* gen,
			BasicChunk* c, bool secondary)
			:chunks(_chunks),generator(gen),chunk(c){}

		void execute()
		{
			chunk->calculateLighting(chunks, second);
		}

		const std::map<BasicChunk*, bool>& chunks;
		BasicChunk* chunk;
		BasicChunkGenerator* generator;
		bool second;
	};

	void addJob(Job* j)
	{
		boost::mutex::scoped_lock lock(mJobMutex);
		mJobs.push_back(j);
		++mPendingJobs;
	}

	void startWorkers()
	{
		boost::mutex::scoped_lock lock(mJobMutex);
		if(!mJobs.empty())
			// just notify one, it'll notify more if need be
			mJobSignal.notify_one();
	}

	void waitForJobs()
	{
		// wait for all current jobs to complete
		boost::mutex::scoped_lock lock(mJobMutex);
		while(mPendingJobs > 0)
			mJobDoneSignal.wait(lock);
	}

	boost::mutex mJobMutex;
	std::deque<Job*> mJobs;
	unsigned int mActiveJobs;
	unsigned int mPendingJobs;

	boost::condition_variable_any mJobSignal;
	boost::condition_variable_any mJobDoneSignal;

	boost::thread_group mThreadPool;

};

#endif
