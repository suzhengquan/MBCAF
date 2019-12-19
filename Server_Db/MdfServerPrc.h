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

#ifndef _MDF_SERVERPRC_H_
#define _MDF_SERVERPRC_H_

#include "MdfPreInclude.h"
#include "MdfSocketServerPrc.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class ServerPrc : public SocketServerPrc
    {
    public:
        ServerPrc(ServerIO * prc, ACE_Reactor * reactor = 0);
        virtual ~ServerPrc();

        /**
        @version 0.9.1
        */
        bool asynMsgPrc(RingBuffer * buffer, Mui32 & size);

        /// @copydetails SocketServerPrc::createInstance
        virtual SocketServerPrc * createInstance(ACE_Reactor * tor) const;

        /// @copydetails ACE_Event_Handler::handle_input
        virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);
    protected:
        ServerPrc();
    };
}
#endif

