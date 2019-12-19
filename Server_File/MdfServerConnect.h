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

#ifndef _MDF_ServerConnect_H_
#define _MDF_ServerConnect_H_

#include "MdfPreInclude.h"
#include "MdfServerIO.h"
#include "MdfTransferTask.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class ServerConnect : public ServerIO
    {
    public:
        ServerConnect();

        virtual ~ServerConnect();

        /**
        @version 0.9.1
        */
        void resetTask();

        /// @copydetails ServerIO::createInstance
        virtual ServerIO * createInstance() const;

        /// @copydetails ServerIO::onConnect
        virtual void onConnect();

        /// @copydetails ServerIO::onClose
        virtual void onClose();

        /// @copydetails ServerIO::onTimer
        virtual void onTimer(TimeDurMS tick);

        /// @copydetails ServerIO::onMessage
        virtual void onMessage(Message * msg);
    private:
        void prcHeartbeat(MdfMessage * msg);
        void PrcClientFileLoginA(MdfMessage * msg);
        void PrcFileState(MdfMessage * msg);
        void PrcClientFilePullA(MdfMessage * msg);
        void PrcClientFilePullQ(MdfMessage * msg);

        int sendNotify(int state, const String & getID, Mui32 user_id, ServerIO * conn);
    private:
        Mui32 mUserID;
        bool mUserCheck;
        TransferTask * mTask;
    };
}
#endif