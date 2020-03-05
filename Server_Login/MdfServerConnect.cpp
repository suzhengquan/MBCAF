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
#include "MdfConnectManager.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect(ACE_Reactor * tor, Mui8 type) :
        ServerIO(tor),
        mType(type)
    {
    }
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect() :
        ServerIO(0)
    {
    }
    //-----------------------------------------------------------------------
    ServerConnect::~ServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    ServerIO * ServerConnect::createInstance() const
    {
        return ServerConnect(mType);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS tick)
    {
        if (mType == ServerType_Client)
        {
            if (tick > mReceiveMark + M_Client_Timeout)
            {
                stop();
            }
        }
        else
        {
            if (tick > mSendMark + M_Heartbeat_Interval)
            {
                MBCAF::ServerBase::Heartbeat msg;
                MdfMessage outmsg;
				outmsg.setProto(&msg);
				outmsg.setCommandID(SBMSG(Heartbeat));
                send(&outmsg);
            }

            if (tick > mReceiveMark + M_Server_Timeout)
            {
                Mlog("connection to MsgServer timeout ");
                stop();
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            break;
        case SBID_ServerInfo:
            prcServerS(temp);
            break;
        case SBID_ConnectNotify:
            prcConnectNotify(temp);
            break;
        case SBID_ServerQ:
            prcServerQ(temp);
            break;
        default:
            Mlog("wrong msg, cmd id=%d ", temp->getCommandID());
            break;
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        M_Only(ConnectManager)->removeServer(mType, getID());
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcServerS(MdfMessage * msg)
    {
        ServerInfo * server = new ServerInfo();
        MBCAF::ServerBase::ServerInfo msg1;
        msg->toProto(&msg1);

        server->mCID = getID();
        server->mType = msg1.type();
        server->mIP = msg1.ip();
        server->mIP2 = msg1.ip2();
        server->mPort = msg1.port();
        server->mPermission = msg1.permission();
        server->mMaxConnect = msg1.mMaxConnect();
        server->mConnectCount = msg1.mConnectCount();
        server->hostname = msg1.host_name();

        M_Only(ConnectManager)->addServer(server->mType, server);

        Mlog("MsgServInfo, mIP=%s, mIP2=%s, mPort=%d, mMaxConnect=%d, mConnectCount=%d, "\
            "hostname: %s. ",
            server->mIP.c_str(), server->mIP2.c_str(),
            server->mPort, server->mMaxConnect,
            server->mConnectCount, server->hostname.c_str());
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcConnectNotify(MdfMessage * msg)
    {
        ServerInfo * sinfo = M_Only(ConnectManager)->getServer(mType, getID());
        if(sinfo)
        {
            MBCAF::ServerBase::ServerUserCount msg;
            msg->toProto(&msg);

            Mui32 action = msg.user_action();
            if (action == ServerConnectInc)
            {
                sinfo->mConnectCount++;
            }
            else
            {
                sinfo->mConnectCount--;
            }

            Mlog("%s:%d, cur_cnt=%u  ", sinfo->hostname.c_str(), sinfo->mPort, sinfo->mConnectCount);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcServerQ(MdfMessage * msg)
    {
        MBCAF::ServerBase::ServerQ msg;
        msg->toProto(&msg);
        Nui8 sertype = msg.type();
        if (M_Only(ConnectManager)->getServerCount(sertype) == 0)
        {
            MBCAF::ServerBase::ServerA msg;
            msg.set_result_code(::MBCAF::Proto::RT_NoServer);
            MdfMessage outmsg;
			outmsg.setProto(&msg);
			outmsg.setCommandID(SBMSG(ServerA));
			outmsg.setSeqIdx(msg->getSeqIdx());
            send(&outmsg);
            stop();
            return;
        }

        ServerInfo * re = M_Only(ConnectManager)->getServer(sertype, true);

        if (re == 0)
        {
            MBCAF::ServerBase::ServerA msg;
            msg.set_result_code(::MBCAF::Proto::RT_ServerBusy);
            MdfMessage outmsg;
			outmsg.setProto(&msg);
			outmsg.setCommandID(SBMSG(ServerA));
			outmsg.setSeqIdx(msg->getSeqIdx());
            send(&outmsg);
        }
        else
        {
            MBCAF::ServerBase::ServerA msg;
            msg.set_result_code(::MBCAF::Proto::RT_OK);
            msg.set_prior_ip(re->mIP);
            msg.set_backip_ip(re->mIP2);
            msg.set_port(re->mPort);
            MdfMessage outmsg;
			outmsg.setProto(&msg);
			outmsg.setCommandID(SBMSG(ServerA));
			outmsg.setSeqIdx(msg->getSeqIdx());
            send(&outmsg);
        }

        stop();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcClientVersionA(MdfMessage * msg)
    {
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcServerListA(MdfMessage * msg)
    {
    }
    //-----------------------------------------------------------------------
}