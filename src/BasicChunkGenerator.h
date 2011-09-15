#ifndef BasicChunkGenerator_H
#define BasicChunkGenerator_H

#include "ChunkGenerator.h"
#include "BasicChunk.h"

class BasicChunkGenerator : public ChunkGenerator
{
public:

	BasicChunkGenerator();
	virtual ~BasicChunkGenerator();

	virtual void update(Real delta);

	virtual void backgroundThread();

	int numGeneratedChunks;
	int numActiveChunks;

	static void workerThread(BasicChunkGenerator* gen);
	void generate();
	void activate();
	void apply();
	void light();
	void build();

private:

	// The chunks themselves
	std::map<InterChunkCoords, BasicChunk*> mChunks;

	// the bool is whether it needs a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mDirtyChunks; 
	boost::mutex mDirtyListMutex;

	// the bool is whether it got a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mBuiltChunks; 
	boost::mutex mBuiltListMutex;

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
