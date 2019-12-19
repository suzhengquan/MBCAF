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

#include "MdfThreadManager.h"

namespace Mdf
{
	M_SingletonImpl(ThreadManager);

	class LambdaTask : public Mdf::ThreadMain
	{
	public:
		LambdaTask(std::function<void()> operationRun) :
			m_operationRun(operationRun)
		{
		}

		virtual void run()
		{
			m_operationRun();
		}

	private:
		std::function<void()> m_operationRun;
	};
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	// ThreadCondition
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	ThreadCondition::ThreadCondition(ACE_Thread_Mutex & mutex) :
		mMutex(mutex),
		mCond(mMutex)
	{

	}
	//--------------------------------------------------------------------------
	ThreadCondition::~ThreadCondition()
	{

	}
	//--------------------------------------------------------------------------
	bool ThreadCondition::wait(uint64_t ms)
	{
		ACE_Time_Value now(ACE_OS::gettimeofday());
		now += ACE_Time_Value(0, ms * 1000);
		if (-1 == mCond.wait(&now))
		{
			return false;
		}
		return true;
	}
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	// Thread
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
    Thread::Thread() :
        mCount(1),
        mCondition(mMutex),
        mExitCondition(mExitMutex),
        mTGroup(-1)
    {
        mTID = (ACE_thread_t *)malloc(sizeof(ACE_thread_t));
        mTHandle = (ACE_hthread_t *)malloc(sizeof(ACE_hthread_t));
    }
    //--------------------------------------------------------------------------
	Thread::Thread(MCount cnt) :
		mCount(cnt),
		mCondition(mMutex),
		mExitCondition(mExitMutex),
		mTGroup(-1)
	{
        mTID = (ACE_thread_t *)malloc(cnt * sizeof(ACE_thread_t));
        mTHandle = (ACE_hthread_t *)malloc(cnt * sizeof(ACE_hthread_t));
	}
	//--------------------------------------------------------------------------
	Thread::~Thread()
	{
        destroy();
        if (mTID)
        {
            free(mTID);
            mTID = 0;
        }
        if (mTHandle)
        {
            free(mTHandle);
            mTHandle = 0;
        }
	}
	//--------------------------------------------------------------------------
	void Thread::activate()
	{
        if (mCount == 1)
        {
            mTGroup = ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(svc),
                (void *)this, THR_NEW_LWP | THR_DETACHED, mTID, mTHandle);
        }
        else if (mCount > 1)
        {
            mTGroup = ACE_Thread_Manager::instance()->spawn_n(mTID, mCount, ACE_THR_FUNC(svc),
                (void *)this, THR_NEW_LWP | THR_DETACHED, ACE_DEFAULT_THREAD_PRIORITY, -1, 0, 0, mTHandle);
        }
	}
	//--------------------------------------------------------------------------
	void Thread::destroy()
	{
		ACE_Thread_Manager::instance()->cancel_grp(mTGroup);

		mCondition.signal();

		mExitCondition.wait();

		mCondition.lock();
		std::list<ThreadMain *>::iterator i, iend = mList.end();
		for (i = mList.begin(); i != iend; ++i)
		{
			delete *i;
		}
		mList.clear();
		mCondition.unlock();
	}
	//--------------------------------------------------------------------------
	void * Thread::svc(void * lpParam)
	{
		Thread * temp = static_cast<Thread *>(lpParam);
		while (ACE_Thread_Manager::instance()->testcancel(temp->mTID[0]) == 0)
		{
			temp->mCondition.lock();

			while (!ACE_Thread_Manager::instance()->testcancel(temp->mTID[0]) &&
				temp->mList.empty())
			{
				temp->mCondition.wait();
			}
			if (ACE_Thread_Manager::instance()->testcancel(temp->mTID[0]))
			{
				temp->mCondition.unlock();
				break;
			}
			ThreadMain * m = temp->mList.front();
			temp->mList.pop_front();
			temp->mCondition.unlock();

			if (ACE_Thread_Manager::instance()->testcancel(temp->mTID[0]) == 0)
			{
				m->run();
			}
			delete m;
		}
		temp->mExitCondition.signal();
		return 0;
	}
	//--------------------------------------------------------------------------
	bool Thread::add(ThreadMain * task)
	{
		mCondition.lock();
		mList.push_back(task);
		mCondition.unlock();

		mCondition.signal();
		return true;
	}
	//--------------------------------------------------------------------------
	bool Thread::add(std::function<void()> func, const std::string & name)
	{
		LambdaTask * task = new LambdaTask(func);
		task->setName(name);

		mCondition.lock();
		mList.push_back(task);
		mCondition.unlock();

		mCondition.signal();
		return true;
	}
	//--------------------------------------------------------------------------
	bool Thread::cancel(ThreadMain * task)
	{
		mCondition.lock();
		std::list<ThreadMain *>::iterator i, iend = mList.end();
		for (i = mList.begin(); i != iend; ++i)
		{
			if (*i == task)
			{
				delete *i;
				mList.erase(i);
				break;
			}
		}
		mCondition.unlock();
		if (i == iend)
			return false;
		return false;
	}
	//--------------------------------------------------------------------------
	bool Thread::cancel(const std::string & name)
	{
		mCondition.lock();
		std::list<ThreadMain *>::iterator i, iend = mList.end();
		for (i = mList.begin(); i != iend; ++i)
		{
			if ((*i)->getName() == name)
			{
				delete *i;
				mList.erase(i);
				break;
			}
		}
		mCondition.unlock();
		if (i == iend)
			return false;
		return true;
	}
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	// ThreadManager
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	ThreadManager::ThreadManager():
		mCustomPoolCount(0)
	{
		ACE::init();
	}
	//--------------------------------------------------------------------------
	ThreadManager::~ThreadManager()
	{
		shutdown();
		ACE_Thread_Manager::instance()->wait();
		ACE::fini();
	}
	//--------------------------------------------------------------------------
	int ThreadManager::setup(Mui32 threadcnt)
	{
		mPool.resize(threadcnt);

		for (uint32_t i = 0; i < threadcnt; ++i)
		{
			mPool[i] = new Thread(1);
			mPool[i]->index(i);
			mPool[i]->activate();
		}

		return 0;
	}
	//--------------------------------------------------------------------------
	Thread * ThreadManager::create(MCount cnt)
	{
		++mCustomPoolCount;
		Thread * tmp = new Thread(cnt);
		mPool.push_back(tmp);
		tmp->activate();
		return tmp;
	}
	//--------------------------------------------------------------------------
	void ThreadManager::destroy(Thread * thread)
	{
		if (mCustomPoolCount == 0)
			return;
		std::vector<Thread *>::iterator i, iend = mPool.end();
		for (i = mPool.begin(); i != mPool.end(); ++i)
		{
			if (*i == thread)
			{
				--mCustomPoolCount;
				delete *i;
				mPool.erase(i);
				break;
			}
		}
	}
	//--------------------------------------------------------------------------
	void ThreadManager::shutdown()
	{
		std::vector<Thread *>::iterator i, iend = mPool.end();
		for (i = mPool.begin(); i != iend; ++i)
		{
			delete *i;
		}
		mPool.clear();
	}
	//--------------------------------------------------------------------------
	void ThreadManager::add(ThreadMain * task)
	{
		uint32_t thread_idx = rand() % (mPool.size() - mCustomPoolCount);
		mPool[thread_idx]->add(task);
	}
	//--------------------------------------------------------------------------
}