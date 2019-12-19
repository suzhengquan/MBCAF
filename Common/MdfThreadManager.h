/*
Copyright (c) "2018-2019", Shenzhen Mindeng Technology Co., Ltd(www.niiengine.com),
		Mindeng Base Communication Application Framework
All rights reserved.
	Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
	Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
	Neither the name of the "ORGANIZATION" nor the names of its contributors may be used
to endorse or promote products derived from this software without specific prior written
permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MDF_THREADPOOL_H_
#define _MDF_THREADPOOL_H_

#include "MdfPreInclude.h"
#include "NetCore.h"

namespace Mdf
{
	/**
	@version 0.9.1
	*/
	class MdfNetAPI ScopeLock
	{
	public:
        ScopeLock(ACE_Thread_Mutex & mutex) : mMutex(mutex) { mMutex.acquire(); }
        ~ScopeLock() { mMutex.release(); }
	private:
		ACE_Thread_Mutex & mMutex;
	};

	/**
	@version 0.9.1
	*/
	class MdfNetAPI ScopeRWLock
	{
	public:
        ScopeRWLock(ACE_RW_Thread_Mutex & mutex, bool rLock = true) : mMutex(mutex) { rLock ? mMutex.acquire_read() : mMutex.acquire_write();}
        ~ScopeRWLock() { mMutex.release(); }
	private:
		ACE_RW_Thread_Mutex & mMutex;
	};

	/**
	@version 0.9.1
	*/
	class MdfNetAPI ThreadCondition
	{
	public:
		ThreadCondition(ACE_Thread_Mutex & mutex);
		~ThreadCondition();
		inline void lock() { mMutex.acquire(); }
		inline void unlock() { mMutex.release(); }
		inline void wait() { mCond.wait(); }
		bool wait(uint64_t ms);
		inline void signal() { mCond.signal(); }
		inline void signalAll() { mCond.broadcast(); }
	private:
		ACE_Thread_Mutex & mMutex;
		ACE_Condition_Thread_Mutex mCond;
	};

	/**
	@version 0.9.1
	*/
	class MdfNetAPI ThreadMain
	{
	public:
		ThreadMain() {}
		virtual ~ThreadMain() {}

		virtual void run() = 0;

		inline void setName(const std::string & name) 
		{ 
			mName = name; 
		}

		inline const std::string & getName() const 
		{ 
			return mName; 
		}
	private:
		std::string mName;
	};

	/**
	@version 0.9.1
	*/
	class MdfNetAPI Thread
	{
	public:
        Thread();
		Thread(MCount cnt);
		~Thread();

		void activate();
		void destroy();
		inline void index(Mui32 idx) { mIndex = idx; }
		static void * svc(void * param);
		bool add(ThreadMain * task);
		bool add(std::function<void()> func, const std::string & name = "__inner");
		bool cancel(ThreadMain * task);
		bool cancel(const std::string & name);
	private:
		Mui32 mIndex;
		int mTGroup;
		MCount mCount;
		ACE_thread_t * mTID;
		ACE_hthread_t * mTHandle;
		ACE_Thread_Mutex mMutex;
		ACE_Thread_Mutex mExitMutex;
		ThreadCondition mCondition;
		ThreadCondition mExitCondition;
		std::list<ThreadMain *> mList;
	};

	/**
	@version 0.9.1
	*/
	class MdfNetAPI ThreadManager
	{
	public:
		ThreadManager();
		virtual ~ThreadManager();

		int setup(Mui32 threadcnt);

		Thread * create(MCount cnt);

		void destroy(Thread * thread);

		void shutdown();
		
		void add(ThreadMain * task);
	private:
		std::vector<Thread *> mPool;
		Mui32 mCustomPoolCount;
	};

	M_SingletonDef(ThreadManager);
}
#endif
