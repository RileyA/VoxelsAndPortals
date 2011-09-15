#include "ThreadPool.h"

ThreadPool::ThreadPool(int numThreads)
	:mNumThreads(numThreads)
{
	for(int i = 0; i < numThreads; ++i)
		mThreadPool.add_thread(new boost::thread(&workerThread, this));
	mActiveJobs = 0;
	mPendingJobs = 0;
}
//---------------------------------------------------------------------------

ThreadPool::~ThreadPool()
{
	// flood the job queue with null jobs...
	for(int i = 0; i < mNumThreads * 2; ++i)
		addJob(0);

	// start the workers (no workers should be working at this point)
	startWorkers();

	//  let them all wrap up
	mThreadPool.join_all();
}
//---------------------------------------------------------------------------

void ThreadPool::addJob(Job* job)
{
	boost::mutex::scoped_lock lock(mJobMutex);
	mJobs.push_back(job);
	++mPendingJobs;
}
//---------------------------------------------------------------------------

void ThreadPool::startWorkers()
{
	boost::mutex::scoped_lock lock(mJobMutex);
	// just notify one, it'll notify more if need be
	if(!mJobs.empty())
		mJobSignal.notify_one();
}
//---------------------------------------------------------------------------

void ThreadPool::waitForWorkers()
{
	// wait for all current jobs to complete
	boost::mutex::scoped_lock lock(mJobMutex);
	while(mPendingJobs > 0)
		mJobDoneSignal.wait(lock);
}
//---------------------------------------------------------------------------

void ThreadPool::workerThread(ThreadPool* pool)
{
	Job* assigned = 0;
	bool moreJobs = false;
	bool hasJob = false; // poor unemployed worker thread.. ;_;

	// the only way it will terminate is if it receives a null job
	while(true)
	{
		// lock to access job queue
		{
			boost::mutex::scoped_lock lock(pool->mJobMutex);

			// skip if this is the first iteration
			if(hasJob)
			{
				// must've just wrapped up a job
				--pool->mActiveJobs;
				--pool->mPendingJobs;

				// notify if there are no jobs left
				if(pool->mPendingJobs == 0)
					pool->mJobDoneSignal.notify_one();
	
				// if the last job was null, then terminate
				if(!assigned)
					return;
				else
					delete assigned;
			}

			// loop on the condition, to handle spurious wakeups
			// note that this will get skipped if jobs remain in the queue after being awoken
			while(pool->mJobs.empty())
				pool->mJobSignal.wait(lock);

			// if it gets here, there must be a job for it...
			assigned = pool->mJobs.front();
			pool->mJobs.pop_front();
			hasJob = true;
			++pool->mActiveJobs;

			moreJobs = !pool->mJobs.empty();
		}

		// now that the mutex is unlocked, if there's still work to be done, notify another thread
		if(moreJobs)
			pool->mJobSignal.notify_one();

		// actually do the work...
		if(assigned)
			assigned->execute();
	}
}
//---------------------------------------------------------------------------
