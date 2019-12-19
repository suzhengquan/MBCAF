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

#include "MdfTransferTaskManager.h"
#include "MdfServerConnect.h"
#include "MdfConnectManager.h"
#include "MdfStrUtil.h"
#include "MBCAF.Proto.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    M_SingletonImpl(TransferManager);
    //-----------------------------------------------------------------------
    TransferManager::TransferManager() :
        mTimeOut(4800)
    {
        setEnable(true);
    }
    //-----------------------------------------------------------------------
    TransferManager::~TransferManager()
    {
        setEnable(false);
    }
    //-----------------------------------------------------------------------
    void TransferManager::setTimeOut(TimeDurMS timeout)
    {
        mTimeOut = timeout;
    }
    //-----------------------------------------------------------------------
    TimeDurMS TransferManager::getTimeOut() const
    {
        return mTimeOut;
    }
    //-----------------------------------------------------------------------
    void TransferManager::addServer(const char * ip, uint16_t port)
    {
        MBCAF::Proto::IPAddress addr;
        addr.set_ip(ip);
        addr.set_port(port);
        mIPList.push_back(addr);
    }
    //-----------------------------------------------------------------------
    const TransferManager::IPList & TransferManager::getServerList() const
    {
        return mIPList;
    }
    //-----------------------------------------------------------------------
    void TransferManager::onTimer(TimeDurMS tick)
    {
        for (TaskList::iterator it = mTaskList.begin(); it != mTaskList.end();)
        {
            TransferTask * task = it->second;
            if (task == NULL)
            {
                mTaskList.erase(it++);
                continue;
            }

            if (task->getState() != TST_WaitingUpload &&
                task->getState() == TST_TransferDone)
            {
                long esp = time(NULL) - task->getCreateTime();
                if (esp > getTimeOut())
                {
                    if(task->getFromConnect())
                    {
                        ServerConnect * conn = reinterpret_cast<ServerConnect*>(task->getFromConnect());
                        conn->resetTask();
                    }
                    if(task->getToConnect())
                    {
                        ServerConnect * conn = reinterpret_cast<ServerConnect*>(task->getToConnect());
                        conn->resetTask();
                    }
                    delete task;
                    mTaskList.erase(it++);
                    continue;
                }
            }
            ++it;
        }
    }
    //-----------------------------------------------------------------------
    TransferTask * TransferManager::createTask(Mui32 trans_mode, 
        const String & id, Mui32 fromuser, Mui32 touser,
            const String & file, Mui32 size)
    {
        TransferTask * re = NULL;

        TaskList::iterator it = mTaskList.find(id);
        if (it == mTaskList.end())
        {
            if (trans_mode == MBCAF::Proto::TFT_Online)
            {
                re = new OnlineTransferTask(id, fromuser, touser, file, size);
            }
            else if (trans_mode == MBCAF::Proto::TFT_Offline)
            {
                re = new OfflineTransferTask(id, fromuser, touser, file, size);
            }
            else
            {
                Mlog("Invalid trans_mode = %d", trans_mode);
            }

            if (re)
            {
                mTaskList.insert(std::make_pair(id, re));
            }
        }
        else
        {
            Mlog("Task existed by id=%s", id.c_str());
        }

        return re;
    }
    //-----------------------------------------------------------------------
    OfflineTransferTask * TransferManager::createTask(const String & id, Mui32 touser)
    {
        OfflineTransferTask * re = OfflineTransferTask::LoadFromDisk(id, touser);
        if (re)
        {
            mTaskList.insert(std::make_pair(id, re));
        }
        return re;
    }
    //-----------------------------------------------------------------------
    bool TransferManager::destroyConnectCloseTask(const String & id)
    {
        bool re = false;

        TaskList::iterator it = mTaskList.find(id);
        if (it != mTaskList.end())
        {
            TransferTask * task = it->second;
            if (task->getMode() == TFT_Online)
            {
                if (task->getFromConnect() == NULL && task->getToConnect() == NULL)
                {
                    delete task;
                    mTaskList.erase(it);
                    re = true;
                }
            }
            else
            {
                if (task->getState() != TST_WaitingUpload)
                {
                    delete task;
                    mTaskList.erase(it);
                    re = true;
                }
            }
        }
        return re;
    }
    //-----------------------------------------------------------------------
    bool TransferManager::destroyTask(const String & id)
    {
        bool rv = false;

        TaskList::iterator it = mTaskList.find(id);
        if (it != mTaskList.end())
        {
            delete it->second;
            mTaskList.erase(it);
        }
        return rv;
    }
    //-----------------------------------------------------------------------
    TransferTask * TransferManager::getTask(const String & id)
    {
        TransferTask * re = NULL;
        TaskList::iterator it = mTaskList.find(id);
        if (it != mTaskList.end())
        {
            re = it->second;
        }

        return re;
    }
    //-----------------------------------------------------------------------
}