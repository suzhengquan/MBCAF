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

#include "MdfServerConnect.h"
#include "MdfSyncCenter.h"
#include "MdfStrUtil.h"
#include "MBCAF.Proto.pb.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    typedef void(*MessagePrc)(ServerConnect * connect, Message * msg);

    class CProxyTask : public ThreadMain
    {
    public:
        CProxyTask(ServerConnect * connect, MessagePrc prc, Message * msg)
        {
            mConnect = connect;
            mMsgPrc = prc;
            mMessage = msg;
        }
        virtual ~CProxyTask()
        {
            if (mMessage)
            {
                delete mMessage;
            }
        }
        void run()
        {
            if (!mMessage)
            {
                M_Only(DataSyncManager)->response(mConnect, mMessage);
            }
            else
            {
                if (mMsgPrc)
                {
                    mMsgPrc(mConnect, mMessage);
                }
            }
        }
    private:
        ServerConnect * mConnect;
        MessagePrc mMsgPrc;
        Message * mMessage;
    };
    //------------------------------------------------------------------
    ServerConnect::ServerConnect()
    {
    }
    //------------------------------------------------------------------
    ServerConnect::~ServerConnect()
    {

    }
    //------------------------------------------------------------------
    ServerIO * ServerConnect::createInstance() const
    {
        reutnr new ServerConnect();
    }
    //------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        setTimer(true, 0, 1000);
        M_Only(ConnectManager)->addServerConnect(ServerType_Server, this);
    }
    //------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        M_Only(ConnectManager)->removeServerConnect(ServerType_Server, this);
    }
    //------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mSendMark + M_Heartbeat_Interval)
        {
            MdfMessage msg;
            MBCAF::ServerBase::Heartbeat proto;
            msg.setProto(&proto);
            msg.setCommandID(SBMSG(Heartbeat));
            send(&msg);
        }

        if (tick > mReceiveMark + M_Server_Timeout)
        {
            Mlog("proxy connection timeout %s:%d", mIP.c_str(), mPort);
            stop();
        }
    }
    //------------------------------------------------------------------
}