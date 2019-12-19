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

#include "MdfDBClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfHttpQuery.h"
#include "MdfConnect.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.MsgServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    static Mui32 mConnectPreServer = 0;
    static ClientReConnect * gDatabaseTimer = 0;
    //-----------------------------------------------------------------------
    void setupDatabaseConnect(const ConnectInfoList & clist, Mui32 connectcnt)
    {
        if (clist.size() == 0)
            return;

        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
        ConnectInfoList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = *i;
            info->mReactor = reactor;
            info->mConnect = new SocketClientPrc(new DataBaseClientConnect(info->mID), reactor);
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
            M_Only(ConnectManager)->addClient(ClientType_DataBase, info);
        }
        M_Only(ConnectManager)->spawnReactor(2, reactor);

        gDatabaseTimer = new ClientReConnect(ClientType_DataBase);
        gDatabaseTimer->setEnable(true);
        
        Mui32 dbconnectcnt = clist.size() / connectcnt;
        mConnectPreServer = (dbconnectcnt / 2) * connectcnt;
    }
    //-----------------------------------------------------------------------
    void shutdownRouteConnect()
    {
        if (gDatabaseTimer)
        {
            delete gDatabaseTimer;
            gDatabaseTimer = 0;
        }
    }
    //-----------------------------------------------------------------------
    DataBaseClientConnect * getClientConnectRange(Mui32 begin, Mui32 end)
    {
        srand(time(NULL));

        DataBaseClientConnect * re = 0;
        const ConnectManager::ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_DataBase);
        ConnectManager::ClientList::iterator i = clist.begin();
        ConnectManager::ClientList::iterator iend = clist.begin();
        std::advance(i, begin);
        std::advance(iend, end);
        for(; i != end; ++i)
        {
            ConnectInfo * temp = i->second;
            if(temp->mConnectState)
            {
                re = static_cast<DataBaseClientConnect *>(temp->mConnect->getBase());
                break;
            }
        }

        if (i == end)
        {
            return 0;
        }

        while (1)
        {
            int c = rand() % (end - begin) + begin;
            ConnectManager::ClientList::iterator j = clist.begin();
            std::advance(j, c);
            ConnectInfo * temp = j->second;
            if(temp->mConnectState)
            {
                re = static_cast<DataBaseClientConnect *>(temp->mConnect->getBase());
                break;
            }
        }

        return re;
    }
    //-----------------------------------------------------------------------
    DataBaseClientConnect * DataBaseClientConnect::getPrimaryConnect()
    {
        DataBaseClientConnect * re = getClientConnectRange(mConnectPreServer, 
            M_Only(ConnectManager)->getClientCount(ClientType_DataBase));
        if (!re)
        {
            re = getClientConnectRange(0, mConnectPreServer);
        }

        return re;
    }
    //-----------------------------------------------------------------------
    DataBaseClientConnect::DataBaseClientConnect(Mui32 idx):
        mIndex(idx)
    {
    }
    //-----------------------------------------------------------------------
    DataBaseClientConnect::~DataBaseClientConnect()
    {

    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::connect(ACE_Reactor * react, const String & ip, uint16_t port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        M_Only(ConnectManager)->addClientConnect(ClientType_DataBase, this);
        M_Only(ConnectManager)->confirmClient(ClientType_DataBase, mIndex);
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::onClose()
    {
        M_Only(ConnectManager)->removeClientConnect(ClientType_DataBase, this);
        M_Only(ConnectManager)->resetClient(ClientType_DataBase, mIndex);
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::onTimer(TimeDurMS tick)
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
            Mlog("conn to db server timeout");
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            break;
        case SBID_StopReceive:
            prcStopReceive(temp);
            break;
        case MSID_CreateGroupA:
            prcCreateGroupA(temp);
            break;
        case MSID_GroupMemberSA:
            prcGroupMemberA(temp);
            break;
        default:
            Mlog("db server, wrong cmd id=%d", temp->getCommandID());
        }
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::prcStopReceive(MdfMessage * msg)
    {
        ConnectInfo * info = M_Only(ConnectManager)->getClient(ClientType_DataBase, mIndex);
        Mlog("HandleStopReceivePacket, from %s:%d", info->mServerIP.c_str(), info->mServerPort);

        stop();
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::prcCreateGroupA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupCreateA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        String gname = proto.group_name();
        Mui32 res = proto.result_code();
        Mui32 gid = 0;
        if (proto.has_group_id())
        {
            gid = proto.group_id();
        }
        HandleExtData extdata((Mui32*)proto.attach_data().c_str(), proto.attach_data().length());
        ServerConnect * sconn = static_cast<ServerConnect *>(M_Only(ConnectManager)->getServerConnect(ServerType_Server, extdata.getHandle()));
        if (!sconn)
        {
            Mlog("no http connection");
            return;
        }
        Mlog("HandleCreateGroupRsp, req_id=%u, group_name=%s, result=%u", userid, gname.c_str(), res);

        RingBuffer temp;
        if (res != 0)
        {
            HttpQuery::genCreateGroupResult(HET_GroupCreate, HTTP_ERROR_MSG[10].c_str(), gid);
        }
        else
        {
            HttpQuery::genCreateGroupResult(HET_OK, HTTP_ERROR_MSG[0].c_str(), gid);
        }
        sconn->send(temp.getBuffer(), temp.getWriteSize());
        sconn->stop();
    }
    //-----------------------------------------------------------------------
    void DataBaseClientConnect::prcGroupMemberA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupMemberSA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 change_type = proto.change_type();
        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        Mui32 gid = proto.group_id();
        Mui32 usercnt = proto.chg_user_id_list_size();
        Mui32 curcnt = proto.cur_user_id_list_size();
        Mlog("HandleChangeMemberResp, change_type=%u, req_id=%u, group_id=%u, result=%u, chg_usr_cnt=%u, cur_user_cnt=%u.",
            change_type, userid, gid, result, usercnt, curcnt);
        HandleExtData extdata((Mui32*)proto.attach_data().c_str(), proto.attach_data().length());
        ServerConnect * sconn = static_cast<ServerConnect *>(M_Only(ConnectManager)->getServerConnect(ServerType_Server, extdata.getHandle()));
        if (!sconn)
        {
            Mlog("no http connection.");
            return;
        }

        RingBuffer temp;
        if (result != 0)
        {
            HttpQuery::genResult(temp, HET_GroupMember, HTTP_ERROR_MSG[11].c_str());
        }
        else
        {
            HttpQuery::genResult(temp, HET_OK, HTTP_ERROR_MSG[0].c_str());
        }
        sconn->send(temp.getBuffer(), temp.getWriteSize());
        sconn->stop();

        if (!result)
        {
            MBCAF::MsgServer::GroupMemberNotify remsg;
            remsg.set_user_id(userid);
            remsg.set_change_type((MBCAF::Proto::GroupModifyType)change_type);
            remsg.set_group_id(gid);
            for(Mui32 i = 0; i < usercnt; i++)
            {
                remsg.add_chg_user_id_list(proto.chg_user_id_list(i));
            }
            for(Mui32 i = 0; i < curcnt; i++)
            {
                remsg.add_cur_user_id_list(proto.cur_user_id_list(i));
            }
            MdfMessage remsg;
            remsg.setProto(&remsg);
            remsg.setCommandID(MSMSG(GroupMemberNotify));
            RouteClientConnect * reconn = RouteClientConnect::getPrimaryConnect();
            if(reconn)
            {
                reconn->send(&remsg);
            }
        }
    }
    //-----------------------------------------------------------------------
}
