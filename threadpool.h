/*-
 * Copyright (c) 2012 Caoimhe Chaos <caoimhechaos@protonmail.com>,
 *                    Ancient Solutions. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions  of source code must retain  the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions  in   binary  form  must   reproduce  the  above
 *    copyright  notice, this  list  of conditions  and the  following
 *    disclaimer in the  documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS  SOFTWARE IS  PROVIDED BY  ANCIENT SOLUTIONS  AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
 * FOR A  PARTICULAR PURPOSE  ARE DISCLAIMED.  IN  NO EVENT  SHALL THE
 * FOUNDATION  OR CONTRIBUTORS  BE  LIABLE FOR  ANY DIRECT,  INDIRECT,
 * INCIDENTAL,   SPECIAL,    EXEMPLARY,   OR   CONSEQUENTIAL   DAMAGES
 * (INCLUDING, BUT NOT LIMITED  TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE,  DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT  LIABILITY,  OR  TORT  (INCLUDING NEGLIGENCE  OR  OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBTRHEADPP_THREADPOOL_H
#define LIBTRHEADPP_THREADPOOL_H 1

#include <google/protobuf/stubs/common.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <pthread.h>

namespace threadpp
{
using std::condition_variable;
using std::mutex;
using std::queue;
using std::string;
using std::unique_lock;
using std::vector;

class ThreadError
{
public:
	explicit ThreadError(int err);
	string String();

protected:
	int err_;
};

// Queue of work items for a thread queue.
class ThreadQueue
{
public:
	// Create a new thread queue.
	ThreadQueue();
	// Delete the thread queue. This will delete any tasks contained
	// inside.
	virtual ~ThreadQueue();

	// Add the closure "t" to the list of tasks to be executed.
	virtual void Add(google::protobuf::Closure* t);

	// Get the next closure to run from the queue.
	virtual google::protobuf::Closure* GetNextTask();

	// Wait until the thread queue is empty, for shutting down.
	// This will leave the queue in a locked state and thus unusable!
	virtual void WaitUntilEmpty();

	// Delete everything from the queue without actually running it.
	virtual void Clear();

protected:
	queue<google::protobuf::Closure*> queued_threads_;
	condition_variable task_availability_;
	mutex queue_lock_;
};

class Thread
{
public:
	// Create a new thread to run the Run() method.
	Thread();

	// Wait for the thread to end and destroy the object.
	virtual ~Thread();

	// Start execution of the thread.
	virtual Thread* Start();

	// Tell the thread to stop processing.
	virtual void Cancel();

	// Delay processing of the current thread and let others take the
	// core.
	static void Yield();

	// Wait until the thread finishes execution.
	virtual void WaitForFinished();

protected:
	// The method to be run when the thread is up.
	virtual void Run() = 0;

	// Enable cancellation.
	virtual void EnableCancellation();

	// Disable cancellation.
	virtual void DisableCancellation();

	// Used by the thread to determine if it was cancelled.
	virtual bool IsCancelled();

	// Helper to call the Run method on the thread.
	static void* CallHelper(void *thread);

	pthread_t thread_;
	bool cancelled_;
};

class ClosureThread : public Thread
{
public:
	// Create a thread which runs the closure given as "c".
	// This will not take ownership of "c".
	explicit ClosureThread(google::protobuf::Closure* c);

	// Cancel the thread.
	virtual ~ClosureThread();

protected:
	// Invokes the closure given to the constructor.
	virtual void Run();

	google::protobuf::Closure* closure_;
};

class WorkerThread : public Thread
{
public:
	// Create a new thread listening for work on the given queue.
	explicit WorkerThread(ThreadQueue* queue);

	// Stop processing and shut down the thread.
	virtual ~WorkerThread();

protected:
	// Called by the thread constructor to retrieve tasks from the
	// queue and run them.
	virtual void Run();

	ThreadQueue* queue_;
};

// Pool of preconfigured threads where given work items can be run.
class ThreadPool
{
public:
	// Create a new thread pool with "num_threads" available threads for
	// concurrent task execution.
	explicit ThreadPool(uint32_t num_threads);

	// Shut down the thread pool. 
	virtual ~ThreadPool();

	// Add the closure "t" to the list of tasks to be executed.
	virtual void Add(google::protobuf::Closure* t);

	// Delete all tasks from the pool.
	virtual void Clear();

protected:
	ThreadQueue tasks_;
	vector<WorkerThread*> threads_;
};
}  // namespace threadpp

#endif /* LIBTRHEADPP_THREADPOOL_H */
