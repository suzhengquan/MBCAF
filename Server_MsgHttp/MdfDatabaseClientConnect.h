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

#ifndef _MDF_DATABASECLIENT_CONNECT_H_
#define _MDF_DATABASECLIENT_CONNECT_H_

#include "MdfPreInclude.h"
#include "MdfClientIO.h"

namespace Mdf
{
    class DataBaseClientTimer;
    /**
    @version 0.9.1
    */
    class DataBaseClientConnect : public ClientIO
    {
    public:
        DataBaseClientConnect(Mui32 idx);
        virtual ~DataBaseClientConnect();

        /**
        @version 0.9.1
        */
        void connect(ACE_Reactor * react, const String & ip, uint16_t port);

        ///@copydetails ClientIO::onConfirm
        virtual void onConfirm();

        ///@copydetails ClientIO::onClose
        virtual void onClose();

        ///@copydetails ClientIO::onTimer
        virtual void onTimer(TimeDurMS tick);

        ///@copydetails ClientIO::onMessage
        virtual void onMessage(Message * msg);

        /**
        @version 0.9.1
        */
        DataBaseClientConnect * getPrimaryConnect();
    public:
        void prcStopReceive(MdfMessage * msg);
        void prcCreateGroupA(MdfMessage * msg);
        void prcGroupMemberA(MdfMessage * msg);
    protected:
        DataBaseClientConnect(){}
    private:
        Mui32 mIndex;
    };

    void setupDatabaseConnect(const ConnectInfoList & clist, Mui32 concnt);
    void shutdownRouteConnect();
}

#endif
