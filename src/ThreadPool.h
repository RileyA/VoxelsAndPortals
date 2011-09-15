#ifndef ThreadPool_H
#define ThreadPool_H

#include "Common.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>


/** A simple and very limited interface that allows you to make a pool of threads
 *	do work from a queue of jobs */
class ThreadPool
{
public:

	/** Constructor
	 *	@param numThreads How many threads in the pool */
	ThreadPool(int numThreads);
	~ThreadPool();

	/** A single unit of work for a thread to do */
	class Job
	{
	public:
		virtual void execute() = 0;
	};

	/** Add a job to the queue
	 *		@param job The job
	 *		@remarks Adding a NULL job will cause the worker that gets
	 *			that job to terminate */
	void addJob(Job* job);

	/** Start the workers working... */
	void startWorkers();

	/** Wait until all of the workers are done */
	void waitForWorkers();

	/** Function used by worker threads */
	static void workerThread(ThreadPool* pool);

protected:

	// mutex that protects the job queue
	boost::mutex mJobMutex;

	// the queue of pending jobs
	std::deque<Job*> mJobs;

	// counters of pending and active jobs
	unsigned int mActiveJobs;
	unsigned int mPendingJobs;

	// how many threads in the pool (fixed atm, may be variable later...)
	unsigned int mNumThreads;

	// used to signal workers that there's work to be done
	boost::condition_variable_any mJobSignal;

	// used to signal that the jobs are done
	boost::condition_variable_any mJobDoneSignal;

	// the pool of threads itself
	boost::thread_group mThreadPool;
	
};

#endif
