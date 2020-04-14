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

#include "MdfClientIO.h"
#include "MdfLogManager.h"

namespace Mdf
{
    //-----------------------------------------------------------------------
    ClientIO::ClientIO(ACE_Reactor * tor, Mui32 idx):
        mReactor(tor ? tor : ACE_Reactor::instance()),
        mIndex(idx),
        mIOPrc(0),
        mMsgPrc(0),
        mMsgBaseIndex(0),
        mStop(true),
        mAbort(false)
    {
    }
    //-----------------------------------------------------------------------
    ClientIO::~ClientIO()
    {
    }
    //-----------------------------------------------------------------------
    ClientIO * ClientIO::createInstance() const
    {
        return new ClientIO(mReactor);
    }
    //-----------------------------------------------------------------------
    void ClientIO::bind(SocketClientPrc * prc)
    {
        mIOPrc = prc;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setIP(const String & ip, Mui16 port)
    {
        mIP = ip;
        mPort = port;
    }
    //-----------------------------------------------------------------------
    const String & ClientIO::getIP() const
    {
        return mIP;
    }
    //-----------------------------------------------------------------------
    Mui16 ClientIO::getPort() const
    {
        return mPort;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setLocalIP(const String & ip, Mui16 port)
    {
        mLocalIP = ip;
        mLocalPort = port;
    }
    //-----------------------------------------------------------------------
    const String & ClientIO::getLocalIP() const
    {
        return mLocalIP;
    }
    //-----------------------------------------------------------------------
    Mui16 ClientIO::getLocalPort() const
    {
        return mLocalPort;
    }
    //-----------------------------------------------------------------------
    Message * ClientIO::create(Mui8 * buf, Mui32 len)
    {
        if (len < sizeof(MdfMsgHeader))
            return 0;

        Mui32 msglen;
        MemStream::read(buf, msglen);
        if (msglen > len)
        {
            return 0;
        }

        MdfMessage * msg = new MdfMessage(buf, msglen, false, true);

        return msg;
    }
    //-----------------------------------------------------------------------
    void ClientIO::destroy(Message * msg)
    {
        delete msg;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setTimer(bool set, TimeDurMS delay, TimeDurMS interval)
    {
        if (mReactor)
        {
            if (set)
            {
                ACE_Time_Value d(0, delay * 1000);
                ACE_Time_Value i(0, interval * 1000);
                mReactor->schedule_timer(mIOPrc, 0, d, i);
            }
            else
            {
                mReactor->cancel_timer(mIOPrc, 1);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ClientIO::setSendSize(Mi32 size)
    {
        int ret = mIOPrc->peer().set_option(SOL_SOCKET, SO_SNDBUF, &size, sizeof(Mi32));
        if (ret == SOCKET_ERROR)
        {
            Mlog("set SO_SNDBUF failed for fd=%d", mIOPrc->peer().get_handle());
        }

        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(SOL_SOCKET, SO_SNDBUF, &re, &resize);
        Mlog("socket=%d send_buf_size=%d", mIOPrc->peer().get_handle(), re);
    }
    //-----------------------------------------------------------------------
    void ClientIO::setRecvSize(Mi32 size)
    {
        int ret = mIOPrc->peer().set_option(SOL_SOCKET, SO_RCVBUF, &size, sizeof(Mi32));
        if (ret == SOCKET_ERROR)
        {
            Mlog("set SO_RCVBUF failed for fd=%d", mIOPrc->peer().get_handle());
        }

        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(SOL_SOCKET, SO_RCVBUF, &re, &resize);
        Mlog("socket=%d recv_buf_size=%d", mIOPrc->peer().get_handle(), re);
    }
    //-----------------------------------------------------------------------
    Mi32 ClientIO::getSendSize() const
    {
        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(SOL_SOCKET, SO_SNDBUF, &re, &resize);
        Mlog("socket=%d send_buf_size=%d", mIOPrc->peer().get_handle(), re);
        return re;
    }
    //-----------------------------------------------------------------------
    Mi32 ClientIO::getRecvSize() const
    {
        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(SOL_SOCKET, SO_RCVBUF, &re, &resize);
        Mlog("socket=%d recv_buf_size=%d", mIOPrc->peer().get_handle(), re);
        return re;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setTTL(Mi32 ttl)
    {
        mIOPrc->peer().set_option(IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    }
    //-----------------------------------------------------------------------
    Mi32 ClientIO::getTTL() const
    {
        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(IPPROTO_IP, IP_TTL, &re, &resize);
        return re;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setTOS(Mi32 tos)
    {
        mIOPrc->peer().set_option(IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
    }
    //-----------------------------------------------------------------------
    Mi32 ClientIO::getTOS() const
    {
        Mi32 resize;
        Mi32 re;
        mIOPrc->peer().get_option(IPPROTO_IP, IP_TOS, &re, &resize);
        return re;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setSendTimeOut(Mi32 time)
    {
#if (defined(_WIN32) || defined(_WIN64))
        time *= 1000;
#endif
        struct timeval timeout = { time, 0 };
        mIOPrc->peer().set_option(SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    }
    //-----------------------------------------------------------------------
    void ClientIO::setRecvTimeOut(Mi32 time)
    {
#if (defined(_WIN32) || defined(_WIN64))
        time *= 1000;
#endif
        struct timeval timeout = { time, 0 };
        mIOPrc->peer().set_option(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
    //-----------------------------------------------------------------------
    void ClientIO::setLinger(bool set, Mi32 time)
    {
        struct linger linger;
        if (set)
        {
#if (defined(_WIN32) || defined(_WIN64))
            time *= 1000;
#endif
            linger.l_onoff = set;
            linger.l_linger = time;
        }
        else
        {
            linger.l_onoff = FALSE;
            linger.l_linger = 0;
        }
        mIOPrc->peer().set_option(SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
    }
    //-----------------------------------------------------------------------
    void ClientIO::setNoDelay(bool set)
    {
        mIOPrc->peer().set_option(IPPROTO_TCP, TCP_NODELAY, &set, sizeof(set));
    }
    //-----------------------------------------------------------------------
    void ClientIO::setMsgBaseIndex(Mi32 idx)
    {
        mMsgBaseIndex = idx;
    }
    //-----------------------------------------------------------------------
    void ClientIO::setMsgQuestionPrc(Mui16 mid, CPrcCB prc)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::setMsgAnswerPrc(Mui16 mid, CPrcCB prc)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::setMsgQandAPrc(Mui16 mid, CPrcCB qprc, CPrcCB aprc)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::unMsgQuestionPrc(Mui16 mid)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::unMsgAnswerPrc(Mui16 mid)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::unMsgQandAPrc(Mui16 mid)
    {

    }
    //-----------------------------------------------------------------------
    void ClientIO::onMessage(Message * msg)
    {
        if(mMsgPrc)
            (this->*mMsgPrc)(msg);
    }
    //-----------------------------------------------------------------------
}

