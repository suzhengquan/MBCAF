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

#include "MdfServerPrc.h"
#include "MdfMessage.h"

/**
 * This macro is used to increase the invocation counter by one when entering
 * handle_input(). It also checks wether the counter is greater than zero
 * indicating, that handle_input() has been called before.
 */
#define INVOCATION_ENTER()                \
{                                        \
    if (mDebugMark > 0)                    \
        logError(ACE_TEXT("Multiple invocations detected.\n")); \
    mDebugMark++;                        \
}

/**
 * THis macro is the counter part to INVOCATION_ENTER(). It decreases the
 * invocation counter and then returns the given value. This macro is
 * here for convenience to decrease the invocation counter also when returning
 * due to errors.
 */
#define INVOCATION_RETURN(retval) { mDebugMark--; return retval; }

namespace Mdf
{
    //-----------------------------------------------------------------------
    ServerPrc::ServerPrc():
        SocketServerPrc()
    {
        M_Trace("ServerPrc::ServerPrc()");
    }
    //-----------------------------------------------------------------------
    ServerPrc::ServerPrc(ServerIO * prc, ACE_Reactor * reactor) :
        SocketServerPrc(prc, reactor)
    {

        M_Trace("ServerPrc::ServerPrc()");
    }
    //-----------------------------------------------------------------------
    ServerPrc::~ServerPrc()
    {
        M_Trace("ServerPrc::~ServerPrc()");
    }
    //-----------------------------------------------------------------------
    SocketServerPrc * ServerPrc::createInstance(ACE_Reactor * tor) const
    {
        ServerIO * base = mBase->createInstance();
        SocketServerPrc * re = new ServerPrc(base, tor);
        base->setup(re);

        return re;
    }
    //-----------------------------------------------------------------------
    int ServerPrc::handle_input(ACE_HANDLE)
    {
        M_Trace("SocketServerPrc::handle_input(ACE_HANDLE)");

        INVOCATION_ENTER();

        while (1)
        {
            Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
            if (bufresever < M_SocketBlockSize)
                mReceiveBuffer.enlarge(M_SocketBlockSize);

            int recsize = mBase->getStream()->recv(mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);
            if (recsize <= 0)
            {
                if (ACE_OS::last_error() != EWOULDBLOCK)
                {
                    INVOCATION_RETURN(-1);
                }
                else
                    break;
            }
            mReceiveBuffer.writeSkip(recsize);
            mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        }


        try
        {
            Mui32 tsize = 0;
            while (asynMsgPrc(&mReceiveBuffer, tsize))
            {
                mReceiveBuffer.readSkip(tsize);
            }
        }
        catch (Exception & ex)
        {
            Mlog("!!!catch exception, err_code=%u, err_msg=%s, close the connection ",
                ex.getMessageType(), ex.getVerbose());
            INVOCATION_RETURN(-1);
        }
        INVOCATION_RETURN(0);
    }
    //------------------------------------------------------------------
    bool ServerPrc::asynMsgPrc(RingBuffer * buffer, Mui32 & size)
    {
        MdfMessage * msg = MdfMessage::create(buffer->getBuffer(), buffer->getWriteSize());
        if (!msg)
            return false;
        else
            size = msg->getSize();

        if (msg->getCommandID() == MBCAF::Proto::SBID_Heartbeat)
        {
            return true;
        }

        MessagePrc prc = M_Only(DataSyncManager)->getPrc(msg->getCommandID());

        if (prc)
        {
            M_Only(DataSyncManager)->getMsgThread()->add(new CProxyTask(this, prc, msg));
        }
        else
        {
            Mlog("no prc for packet type: %d", msg->getCommandID());
        }
        return true;
    }
    //-----------------------------------------------------------------------
}