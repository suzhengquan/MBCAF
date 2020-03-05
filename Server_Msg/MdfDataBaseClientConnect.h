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

#ifndef _MDF_DataBaseClientConnect_H_
#define _MDF_DataBaseClientConnect_H_

#include "MdfPreInclude.h"
#include "MdfClientIO.h"

namespace Mdf
{
    class DataBaseClientConnect : public ClientIO
    {
    public:
        DataBaseClientConnect(ACE_Reactor * tor,Mui32 idx);
        virtual ~DataBaseClientConnect();

        /**
        @version 0.9.1
        */
        void connect(const String & ip, Mui16 port);
        
        /// @copydetails ClientIO::getType
        Mui8 getType() const {return ClientType_DataBase; }

        /// @copydetails ClientIO::onConfirm
        virtual void onConfirm();

        /// @copydetails ClientIO::onTimer
        virtual void onTimer(TimeDurMS tick);

        /// @copydetails ClientIO::onMessage
        virtual void onMessage(Message * msg);
    private:
        void prcUserLoginValidA(MdfMessage * msg);
        void prcBubbyRecentSessionA(MdfMessage * msg);
        void prcBuddyObjectListA(MdfMessage * msg);
        void prcMsgListA(MdfMessage * msg);
        void prcMsgA(MdfMessage * msg);
        void prcMsgData(MdfMessage * msg);
        void prcUnReadCountA(MdfMessage * msg);
        void prcRecentMsgA(MdfMessage * msg);
        void prcBubbyObjectInfoA(MdfMessage * msg);
        void prcStopReceive(MdfMessage * msg);
        void prcBubbyRemoveSessionA(MdfMessage * msg);
        void prcBuddyAvatarA(MdfMessage * msg);
        void prcBuddyChangeSignatureA(MdfMessage * msg);
        void prcTrayMsgA(MdfMessage * msg);
        void prcSwitchTrayMsgA(MdfMessage * msg);
        void prcBuddyOrganizationA(MdfMessage * msg);
        void prcPushShieldSA(MdfMessage * msg);
        void prcPushShieldA(MdfMessage * msg);
    public:
        static DataBaseClientConnect * getSlaveConnect();
        static DataBaseClientConnect * getPrimaryConnect();
    protected:
        DataBaseClientConnect() {}
    private:
        Mui32 mIndex;
    };

    void setupDataBaseConnect(const ConnectInfoList & clist, Mui32 connectcnt, const String & encryptstr);
    void shutdownDataBaseConnect();
}
#endif
