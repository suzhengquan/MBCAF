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
#include "MdfHttpQuery.h"

/**
 * This macro is used to increase the invocation counter by one when entering
 * handle_input(). It also checks wether the counter is greater than zero
 * indicating, that handle_input() has been called before.
 */
#define INVOCATION_ENTER()              \
{                                       \
    if (mDebugMark > 0)                 \
        logError(ACE_TEXT("Multiple invocations detected.\n")); \
    mDebugMark++;                       \
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
        M_Trace("ServerPrc::handle_input(ACE_HANDLE)");

        INVOCATION_ENTER();

        while(1)
        {
            Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
            if(bufresever < M_SocketBlockSize + 1)
                mReceiveBuffer.enlarge(M_SocketBlockSize + 1);

            int recsize = mBase->getStream()->recv(mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);
            if (recsize <= 0)
            {
                if(ACE_OS::last_error() != EWOULDBLOCK)
                    INVOCATION_RETURN(-1);
                else
                    break;
            }
            mReceiveBuffer.writeSkip(recsize);
            mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        }

        char * srcbuf = (char*)mReceiveBuffer.getBuffer();
        Mui32 ssize = mReceiveBuffer.getWriteSize();
        srcbuf[ssize] = '\0';

        mHttpParser.parse(srcbuf, ssize);

        if(mHttpParser.isComplete())
        {
            String url = mHttpParser.getUrl();
            if (strncmp(url.c_str(), "/query/", 7) == 0)
            {
                String content = mHttpParser.getBodyContent();
                HttpQuery * hquery = M_Only(HttpQuery);
                hquery->query(url, content, this);
            }
            else
            {
                Mlog("url unknown, url=%s ", url.c_str());
                INVOCATION_RETURN(-1);
            }
        }
        INVOCATION_RETURN(0);
    }
    //-----------------------------------------------------------------------
    int ServerPrc::handle_output(ACE_HANDLE fd)
    {
        M_Trace("ServerPrc::handle_output(ACE_HANDLE)");
        INVOCATION_ENTER();

    ContinueTransmission:
        ACE_Message_Block * mb = 0;
        ACE_Time_Value nowait(ACE_OS::gettimeofday());
        while (0 <= mOutQueue.dequeue_head(mb, &nowait))
        {
            int sndsize = mBase->getStream()->send(mb->rd_ptr(), mb->length());
            if (sndsize <= 0)
            {
                if (ACE_OS::last_error() != EWOULDBLOCK)
                {
                    INVOCATION_RETURN(-1);
                }
                else
                {
                    mOutQueue.enqueue_head(mb);
                    break;
                }
            }
            else
            {
                mb->rd_ptr((size_t)dsedsize);
            }
            if (mb->length() > 0)
            {
                mOutQueue.enqueue_head(mb);
                break;
            }
            mb->release();
            mSendMark = M_Only(ConnectManager)->getTimeTick();
        }
        if (mBase->isStop())
        {
            if (!mOutQueue.is_empty())
            {
                ACE_OS::sleep(1);
                goto ContinueTransmission;
            }
            INVOCATION_RETURN(-1);
        }
        {
            ScopeLock tlock(mOutMute);
            if (mOutQueue.is_empty())
                mReactor->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
            else
                mReactor->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
        }
        INVOCATION_RETURN(0);
    }
    //-----------------------------------------------------------------------
}