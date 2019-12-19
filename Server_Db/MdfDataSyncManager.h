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

#ifndef _MDF_DATASYNCCenter_H_
#define _MDF_DATASYNCCenter_H_

#include "MdfPreInclude.h"
#include "MdfStrUtil.h"
#include "MdfThreadManager.h"
#include "MdfConnectManager.h"
#include "MBCAF.Proto.pb.h"

namespace Mdf
{
    struct ConnectMessage;
    typedef map<Mui32, MessagePrc> MsgPrcList;

    class DataSyncManager : public ACE_Event_Handler, public ThreadMain, public ConnectManager::TimerListener
    {
    public:
        DataSyncManager();
        ~DataSyncManager();

        /**
        @version 0.9.1
        */
        void setUserUpdate(Mui32 nUpdated);

        /**
        @version 0.9.1
        */
        Mui32 getUserUpdate();

        /**
        @version 0.9.1
        */
        MessagePrc getPrc(Mui32 hid);

        /**
        @version 0.9.1
        */
        void setupMsgThread(MCount cnt);

        /**
        @version 0.9.1
        */
        Thread * getMsgThread() const;

        /**
        @version 0.9.1
        */
        void response(ServerConnect * connect, Message * msg);

        ///@copydetails ThreadMain::run
        void run();
    protected:
        /// @copydetails TimerListener::onTimer
        void onTimer(TimeDurMS tick);
        
        /**
        @version 0.9.1
        */
        void updateGroup(Mui32 time);
    private:
        Thread * mMsgThread;
        Thread * mSyncThread;
        MsgPrcList mMsgPrcList;
        Mui32 mLastGroup;
        Mui32 mLastUser;
        ACE_Thread_Mutex mGroupMutex;
        ThreadCondition * mGroupCondition;
        ACE_Thread_Mutex mUpdateMute;
        ACE_Thread_Mutex mResponseMutex;
        list<ConnectMessage *> mResponseList;
        bool mSyncGroup;
    };

    M_SingletonDef(DataSyncManager);
}
#endif