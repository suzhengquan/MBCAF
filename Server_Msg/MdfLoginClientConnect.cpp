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

#include "MdfLoginClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfUserManager.h"
#include "MdfConnect.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //------------------------------------------------------------------------
    static ClientReConnect * gLoginTimer = 0;
    //------------------------------------------------------------------------
    void setupLoginConnect(const ConnectInfoList & clist, const String & primaryip,
        const String & slaveip, Mui16 port, Mui32 maxconn)
    {
        if (clist.size() == 0)
            return;

        LoginClientConnect::setMessageServer(primaryip, slaveip, port, maxconn);

        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
        ConnectInfoList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = *i;
            info->mReactor = reactor;
            info->mConnect = new SocketClientPrc(new LoginClientConnect(info->mID), reactor);
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
            M_Only(ConnectManager)->addClient(ClientType_Login, info);
        }
        M_Only(ConnectManager)->spawnReactor(2, reactor);

        gLoginTimer = new ClientReConnect(ClientType_Login);
        gLoginTimer->setEnable(true);
    }
    //-----------------------------------------------------------------------
    void shutdownLoginConnect()
    {
        if (gLoginTimer)
        {
            delete gLoginTimer;
            gLoginTimer = 0;
        }
    }
    //------------------------------------------------------------------------
    LoginClientConnect::LoginClientConnect(Mui32 idx):
        mIndex(idx)
    {
    }
    //------------------------------------------------------------------------
    LoginClientConnect::~LoginClientConnect()
    {

    }
    //------------------------------------------------------------------------
    void LoginClientConnect::connect(const char * ip, Mui16 serverPort, Mui32 serv_idx)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //------------------------------------------------------------------------
    void LoginClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        M_Only(ConnectManager)->addClientConnect(ClientType_Login, this);
        M_Only(ConnectManager)->confirmClient(ClientType_Login, mIndex);
        setTimer(true, 0, 1000);

        vector<ConnectID> idlist;
        vector<Mui32> connlist;
        Mui32 cur_conn_cnt = 0;
        M_Only(UserManager)->getUserConnectList(idlist, connlist, cur_conn_cnt);
        char hostname[256] = { 0 };
        gethostname(hostname, 256);
        MBCAF::ServerBase::ServerInfo msg;
        msg.set_ip1(mMsgPrimaryIP);
        msg.set_ip2(mMsgSlaveIP);
        msg.set_port(mMsgPort);
        msg.set_max_conn_cnt(mMsgMaxConnect);
        msg.set_cur_conn_cnt(cur_conn_cnt);
        msg.set_host_name(hostname);
        MdfMessage remsg;
		remsg.setProto(&msg);
		remsg.setCommandID(SBMSG(ServerInfo));
        send(&remsg);
    }
    //------------------------------------------------------------------------
    void LoginClientConnect::onClose()
    {
        M_Only(ConnectManager)->removeClientConnect(ClientType_Login, this);
        M_Only(ConnectManager)->resetClient(ClientType_Login, mIndex);
    }
    //------------------------------------------------------------------------
    void LoginClientConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mSendMark + M_Heartbeat_Interval)
        {
            MBCAF::ServerBase::Heartbeat proto;
            MdfMessage remsg;
			remsg.setProto(&proto);
			remsg.setCommandID(SBMSG(Heartbeat));
            send(&remsg);
        }

        if (tick > mReceiveMark + M_Server_Timeout)
        {
            Mlog("conn to login server timeout ");
            stop();
        }
    }
    //------------------------------------------------------------------------
    void LoginClientConnect::onMessage(Message * msg)
    {
    }
    //------------------------------------------------------------------------
    LoginClientConnect * LoginClientConnect::getPrimaryConnect()
    {
        const ConnectManager::ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_Login);
        ConnectManager::ClientList::const_iterator i, iend = clist.end();
        for(i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = i->second;
            if (info->mConnectState)
            {
                return static_cast<FileClientConnect *>(info->mConnect->getBase());
            }
        }

        return 0;
    }
    //------------------------------------------------------------------------
    LoginClientConnect::setMessageServer(const String & primaryip, const String & slaveip, Mui16 port, Mui32 maxconn)
    {
        mMsgPrimaryIP = primaryip;
        mMsgSlaveIP = slaveip;
        mMsgPort = port;
        mMsgMaxConnect = maxconn;
    }
    //------------------------------------------------------------------------
}