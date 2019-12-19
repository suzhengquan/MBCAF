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

#ifndef _MDF_TRANSFERTASKMANAGER_H_
#define _MDF_TRANSFERTASKMANAGER_H_

#include "MdfPreInclude.h"
#include "MdfTransferTask.h"
#include "MdfConnectManager.h"
#include "MBCAF.Proto.pb.h"

namespace Mdf
{
    /*
    @version 0.9.1
    */
    class TransferManager : public ACE_Event_Handler : public ConnectManager::TimerListener
    {
    public:
        typedef std::list<MBCAF::Proto::IPAddress> IPList;
    public:
        ~TransferManager();

        /*
        @version 0.9.1
        */
        void setTimeOut(TimeDurMS timeout);

        /*
        @version 0.9.1
        */
        TimeDurMS getTimeOut() const;

        /*
        @version 0.9.1
        */
        void addServer(const char * ip, uint16_t port);

        /*
        @version 0.9.1
        */
        const IPList & getServerList() const;

        /*
        @version 0.9.1
        */
        TransferTask * createTask(Mui32 mode, const String & id, 
            Mui32 fromuser, Mui32 touser, const String & file, Mui32 size);

        /*
        @version 0.9.1
        */
        OfflineTransferTask * createTask(const String & id, Mui32 touser);

        /*
        @version 0.9.1
        */
        bool destroyTask(const String & id);

        /*
        @version 0.9.1
        */
        bool destroyConnectCloseTask(const String & id);

        /*
        @version 0.9.1
        */
        TransferTask * getTask(const String & id);
    protected:        
        TransferManager();
        
        /// @copydetails TimerListener::onTimer
        void onTimer(TimeDurMS tick);
    private:
        TaskList mTaskList;
        IPList mIPList;
        TimeDurMS mTimeOut;
    };

    M_SingletonDef(TransferManager);
}
#endif
