#ifndef BasicChunkGenerator_H
#define BasicChunkGenerator_H

#include "ChunkGenerator.h"
#include "BasicChunk.h"
#include "ThreadPool.h"

class BasicChunkGenerator : public ChunkGenerator
{
public:

	BasicChunkGenerator();
	virtual ~BasicChunkGenerator();

	/** Updates meshes that have been built, etc runs on the main thread */
	virtual void update(Real delta);

	/** The background thread loop */
	virtual void backgroundThread();

	/** Get the number of generated chunks
	 *	@remarks Since this is being used for nothing more than a debug overlay
	 *		this isn't protected by a mutex, so no guarantees that it won't explode... */
	int getNumGeneratedChunks(){return mNumGeneratedChunks;}

	/** Get the number of active chunks
	 *	@remarks Since this is being used for nothing more than a debug overlay
	 *		this isn't protected by a mutex, so no guarantees that it won't explode... */
	int getNumActiveChunks(){return mNumActiveChunks;}

	/** Notify that a chunk's lighting has been changed and needs an update */
	void notifyChunkLightChange(BasicChunk* c);

protected:

	/** Generates any needed chunks */
	virtual void generate();

	/** Activates any needed chunks */
	virtual void activate();

	/** Apply changes to chunks */
	virtual void apply();

	/** Do lighting calculations */
	void light();

	/** Build chunk meshes */
	void build();

	// The chunks themselves
	std::map<InterChunkCoords, BasicChunk*> mChunks;

	// the bool is whether it needs a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mDirtyChunks; 
	boost::mutex mDirtyListMutex;

	// the bool is whether it got a full build (just lighting otherwise)
	std::map<BasicChunk*, bool> mBuiltChunks; 
	boost::mutex mBuiltListMutex;

	// thread pool for multithreaded chunk calculations
	ThreadPool* mThreadPool;

	// Total number of chunks generated
	int mNumGeneratedChunks;

	// Number of currently active chunks
	int mNumActiveChunks;

	//---------------------------------------------------------------------------
	/** A ThreadPool Job for building a Chunk's mesh */
	class BuildJob : public ThreadPool::Job
	{
	public:

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
	//---------------------------------------------------------------------------
	/** A ThreadPool Job for calculating a Chunk's lighting */
	class LightJob : public ThreadPool::Job 
	{
	public:

		LightJob(const std::map<BasicChunk*, bool>& _chunks,
			BasicChunkGenerator* gen,BasicChunk* c, bool secondary)
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
	//---------------------------------------------------------------------------
};

#endif
