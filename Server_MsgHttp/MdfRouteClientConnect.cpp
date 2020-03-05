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

#include "MdfRouteClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfConnectManager.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    RouteClientConnect * RouteClientConnect::mPrimaryConnect = 0;
    static ClientReConnect * gRouteTimer = 0;
    //-----------------------------------------------------------------------
    void setupRouteConnect(const ConnectInfoList & clist)
    {
        if (clist.size() == 0)
            return;

        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        ConnectInfoList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = *i;
            info->mReactor = reactor;
            info->mConnect = new SocketClientPrc(new RouteClientConnect(info->mID), reactor);
            ACE_INET_Addr serIP(info->mServerPort, info->mServerIP.c_str());
            ACE_Connector<SocketClientPrc, ACE_SOCK_CONNECTOR> connector;
            if (connector.connect(info->mConnect, serIP) == -1)
            {
                info->mConnectState = false;
            }
            else
            {
                info->mCurrentCount = 0;
            }
            M_Only(ConnectManager)->addClient(ClientType_Route, info);
        }
        M_Only(ConnectManager)->spawnReactor(2, reactor);

        gRouteTimer = new ClientReConnect(ClientType_Route);
        gRouteTimer->setEnable(true);
    }
    //-----------------------------------------------------------------------
    void shutdownRouteConnect()
    {
        if (gRouteTimer)
        {
            delete gRouteTimer;
            gRouteTimer = 0;
        }
    }
    //-----------------------------------------------------------------------
    void RouteClientConnect::checkPrimary()
    {
        Mui64 earlytime = (Mui64)-1;
        ConnectInfo * primary = NULL;
        const ConnectManager::ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_Route);
        ConnectManager::ClientList::const_iterator i, iend = clist.end();
        for(i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * re = i->second;
            if(i->second->mConnectState && (re->mConfirmTime < earlytime))
            {
                primary = re;
                earlytime = re->mConfirmTime;
            }
        }

        mPrimaryConnect = primary;

        if(mPrimaryConnect) 
        {
            MdfMessage msg;
            MBCAF::ServerBase::ServerPrimaryS proto;
            proto.set_master(1);
            msg.setProto(&proto);
            msg.setCommandID(SBMSG(ServerPrimaryS));
            mPrimaryConnect->send(&msg);
        }
    }
    //-----------------------------------------------------------------------
    RouteClientConnect * RouteClientConnect::getPrimaryConnect()
    {
        return mPrimaryConnect;
    }
    //-----------------------------------------------------------------------
    RouteClientConnect::RouteClientConnect(ACE_Reactor * tor, Mui32 idx):
        ClientConnect(tor, idx)
    {
    }
    //-----------------------------------------------------------------------
    RouteClientConnect::~RouteClientConnect()
    {
    }
    //-----------------------------------------------------------------------
    void RouteClientConnect::connect(ACE_Reactor * react, const String & ip, uint16_t port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //-----------------------------------------------------------------------
    void RouteClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        setTimer(true, 0, 1000);

        if(mPrimaryConnect == NULL) 
        {
            checkPrimary();
        }
    }
    //-----------------------------------------------------------------------
    void RouteClientConnect::onClose()
    {
        if(mPrimaryConnect == this)
        {
            checkPrimary();
        }
    }
    //-----------------------------------------------------------------------
    void RouteClientConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mSendMark + M_Heartbeat_Interval) 
        {
            MBCAF::ServerBase::Heartbeat proto;
            MdfMessage msg;
            msg.setProto(&proto);
            msg.setCommandID(SBMSG(Heartbeat));
            send(&msg);
        }

        if (tick > mReceiveMark + M_Server_Timeout) 
        {
            Mlog("conn to route server timeout ");
            stop();
        }
    }
    //-----------------------------------------------------------------------
}