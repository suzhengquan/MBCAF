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
#include "MdfObjConnectInfo.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.Proto.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    typedef hash_map<Mui32, ObjConnectInfo *> ConnectInfoList;
    static ConnectInfoList gInfoList;
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect(ACE_Reactor * tor):
        ServerIO(tor)
    {
        mPrimary = false;
    }
    //-----------------------------------------------------------------------
    ServerConnect::~ServerConnect()
    {

    }
    //-----------------------------------------------------------------------
    ServerIO * ServerConnect::createInstance() const
    {
        return ServerConnect();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS tick)
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
            stop();
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
        case SBID_ObjectInfo:
            prcObjectInfo(temp);
            break;
        case SBID_ObjectInfoS:
            prcObjectStateS(temp);
            break;
            break;
        case MSID_BuddyObjectStateQ:
            prcObjectStateA(temp);
            break;
        case MSID_Data:
        case SBID_P2P:
        case MSID_ReadNotify:
        case SBID_ServerKickObject:
        case MSID_GroupMemberNotify:
        case HSID_FileNotify:
        case MSID_BuddyRemoveSessionS:
            broadcastMsg(temp, this);
            break;
        case MSID_BuddyObjectStateS:
            broadcastMsg(temp);
            break;
        case SBID_ServerPrimaryS:
            prcServerPrimaryS(temp);
        default:
            Mlog("ServerConnect::onMessage, wrong cmd id: %d ", temp->getCommandID());
            break;
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        ConnectInfoList::iterator temp;
        ConnectInfoList::iterator it, itend = gInfoList.end();
        for(it = gInfoList.begin(); it != itend;)
        {
            temp = it++;
            ObjConnectInfo * obj = temp->second;
            obj->removeLogin(this);
            if (obj->getLoginCount() == 0)
            {
                delete obj;
                gInfoList.erase(temp);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcObjectInfo(MdfMessage * msg)
    {
        MBCAF::ServerBase::ObjectConnectInfo msg;
        if (msg->toProto(&msg))
        {

            Mui32 count = msg.user_stat_list_size();
            for (Mui32 i = 0; i < count; ++i)
            {
                MBCAF::Proto::ServerObjectState server_user_stat = msg.user_stat_list(i);
                updateObjectLogin(server_user_stat.user_id(), server_user_stat.status(), server_user_stat.client_type());
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcObjectStateS(MdfMessage * msg)
    {
        MBCAF::ServerBase::ObjectConnectState msg;
        if(!msg->toProto(&msg))
            return;

        Mui32 objstate = msg.user_status();
        Mui32 objid = msg.user_id();
        Mui32 logintype = msg.client_type();
        Mlog("prcObjectStateS, state=%u, oid=%u, login_type=%u ", objstate, objid, logintype);

        updateObjectLogin(objid, objstate, logintype);

        ConnectInfoList::iterator it = gInfoList.find(objid);
        if (it != gInfoList.end())
        {
            ObjConnectInfo * obj = it->second;

            MBCAF::ServerBase::LoginStateNotify msg2;
            msg2.set_user_id(objid);
            if (objstate == MBCAF::Proto::OST_Offline)
            {
                msg2.set_login_status(PL_StateOff);
            }
            else
            {
                msg2.set_login_status(PL_StateOn);
            }
            MdfMessage pdu;
            pdu.setProto(&msg2);
            pdu.setCommandID(SBMSG(LoginStateNotify));

            if (objstate == OST_Offline)
            {
                if (M_PCLoginCheck(logintype) && !obj->isPCLogin())
                {
                    broadcastMsg(&pdu);
                }
            }
            else
            {
                if (obj->isPCLogin())
                {
                    broadcastMsg(&pdu);
                }
            }
        }

        if (M_PCLoginCheck(logintype))
        {
            if (obj && OST_Offline == objstate && obj->isPCLogin())
            {
                return;
            }
            else
            {
                MBCAF::MsgServer::BubbyLoginStateNotify msg3;
                MBCAF::Proto::ObjectState * user_stat = msg3.mutable_user_stat();
                user_stat->set_user_id(objid);
                user_stat->set_status((MBCAF::Proto::ObjectStateType)objstate);
                MdfMessage pdu2;
                pdu2.setProto(&msg3);
                pdu2.setCommandID(MSMSG(BubbyLoginStateNotify));
                broadcastMsg(&pdu2);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcServerPrimaryS(MdfMessage * msg)
    {
        MBCAF::ServerBase::ServerPrimaryS msg;
        if (!msg->toProto(&msg))
        {
            Mui32 master = msg.master();

            Mlog("HandleRoleSet, master=%u, handle=%u ", master, m_handle);
            if (master == 1)
            {
                mPrimary = true;
            }
            else
            {
                mPrimary = false;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcObjectStateA(MdfMessage * msg)
    {
        MBCAF::MsgServer::BuddyObjectStateQ msg;
        if(!msg->toProto(&msg))
            return;

        Mui32 request_id = msg.user_id();
        Mui32 query_count = msg.user_id_list_size();
        Mlog("HandleObjectStateusReq, req_id=%u, query_count=%u ", request_id, query_count);

        MBCAF::MsgServer::BuddyObjectStateA msg2;
        msg2.set_user_id(request_id);
        msg2.set_attach_data(msg.attach_data());

        for (Mui32 i = 0; i < query_count; ++i)
        {
            MBCAF::Proto::ObjectState * ostate = msg2.add_user_stat_list();
            Mui32 user_id = msg.user_id_list(i);
            ostate->set_user_id(user_id);

            ConnectInfoList::iterator it = gInfoList.find(user_id);
            if (it != gInfoList.end())
            {
                ostate->set_status((::MBCAF::Proto::ObjectStateType) it->second->getState());
            }
            else
            {
                ostate->set_status(OST_Offline);
            }
        }

        MdfMessage outmsg;
		outmsg.setProto(&msg2);
		outmsg.setCommandID(MSMSG(BuddyObjectStateA));
		outmsg.setSeqIdx(msg->getSeqIdx());
        send(&outmsg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::updateObjectLogin(Mui32 objID, Mui32 state, Mui32 client_type)
    {
        ConnectInfoList::iterator it = gInfoList.find(objID);
        if (it != gInfoList.end())
        {
            ObjConnectInfo * obj = it->second;
            if (obj->isLoginExist(this))
            {
                if (state == OST_Offline)
                {
                    obj->removeLoginType(client_type);
                    if (obj->getLoginTypeList().empty())
                    {
                        obj->removeLogin(this);
                        if (obj->getLoginCount() == 0)
                        {
                            delete obj;
                            gInfoList.erase(objID);
                        }
                    }
                }
                else
                {
                    obj->addLoginType(client_type);
                }
            }
            else
            {
                if (state != OST_Offline)
                {
                    obj->addLogin(this);
                    obj->addLoginType(client_type);
                }
            }
        }
        else
        {
            if (state != OST_Offline)
            {
                ObjConnectInfo* objInfo = new ObjConnectInfo();
                if (objInfo != NULL)
                {
                    objInfo->addLogin(this);
                    objInfo->addLoginType(client_type);
                    gInfoList.insert(make_pair(objID, objInfo));
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::broadcastMsg(MdfMessage * msg, ServerConnect * sender)
    {
        const ConnectManager::ConnectList & temp = M_Only(ConnectManager)->getServerConnectList(ServerType_Server);
        ConnectManager::ConnectList::const_iterator i, iend = temp.end();
        for (i = temp.begin(); i != iend; ++i)
        {
            ServerConnect * dst = (ServerConnect *)i->second;
            if (dst != sender)
            {
                dst->send(msg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::sendMsgToObject(Mui32 objID, MdfMessage * msg, bool loopback)
    {
        ConnectInfoList::iterator it = gInfoList.find(objID);
        if (it != gInfoList.end())
        {
            ObjConnectInfo * obj = it->second;

            const ObjConnectInfo::ConnectList & loginlist = obj->getLoginList();
            ObjConnectInfo::ConnectList::const_iterator i, iend = loginlist.end();
            for (i = loginlist->begin(); i != iend; ++i)
            {
                ServerConnect * temp = *i;
                if (loopback || temp != this)
                {
                    temp->send(msg->getBuffer(), msg->getSize());
                }
            }
        }
    }
    //-----------------------------------------------------------------------
}