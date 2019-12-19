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
#include "MdfLoginClientConnect.h"
#include "MdfDatabaseClientConnect.h"
#include "MdfPushClientConnect.h"
#include "MdfFileClientConnect.h"
#include "MdfUserManager.h"
#include "MdfFileManager.h"
#include "MdfGroupManager.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.HubServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //------------------------------------------------------------------------
    RouteClientConnect * RouteClientConnect::mPrimaryConnect = 0;
    static ClientReConnect * gRouteTimer = 0;
    //------------------------------------------------------------------------
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
    //------------------------------------------------------------------------
    void RouteClientConnect::checkPrimary()
    {
        Mui64 earlytime = (Mui64)-1;
        ConnectInfo * primary = NULL;
        const ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_Route);
        ClientList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * re = i->second;
            if (i->second->mConnectState && (re->mConfirmTime < earlytime))
            {
                primary = re;
                earlytime = re->mConfirmTime;
            }
        }

        mPrimaryConnect = primary;

        if (mPrimaryConnect)
        {
            MBCAF::ServerBase::ServerPrimaryS proto;
            proto.set_master(1);
            MdfMessage remsg;
			remsg.setProto(&proto);
			remsg.setCommandID(SBMSG(ServerPrimaryS));
            mPrimaryConnect->send(&remsg);
        }
    }
    //------------------------------------------------------------------------
    RouteClientConnect::RouteClientConnect(Mui32 idx):
        mIndex(idx)
    {
    }
    //------------------------------------------------------------------------
    RouteClientConnect::~RouteClientConnect()
    {
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::connect(const String & ip, Mui16 port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        M_Only(ConnectManager)->addClientConnect(ClientType_Route, this);
        M_Only(ConnectManager)->confirmClient(ClientType_Route, mIndex);
        setTimer(true, 0, 1000);

        if (mPrimaryConnect == NULL)
        {
            checkPrimary();
        }

        list<UserState> userlist;
        M_Only(UserManager)->getUserInfoList(userlist);
        MBCAF::ServerBase::ObjectConnectInfo proto;
        list<UserState>::iterator it, itend = userlist.end();
        for (it = userlist.begin(); it != itend; ++it)
        {
            MBCAF::Proto::ServerObjectState* server_user_stat = proto.add_user_stat_list();
            server_user_stat->set_user_id((*it).mUserID);
            server_user_stat->set_status((::MBCAF::Proto::ObjectStateType)(*it).mState);
            server_user_stat->set_client_type((::MBCAF::Proto::ClientType)(*it).mClientType);
        }

        MdfMessage remsg;
        remsg.setProto(&proto);
        remsg.setCommandID(SBMSG(ObjectInfo));
        send(&remsg);
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::onClose()
    {
        M_Only(ConnectManager)->removeClientConnect(ClientType_Route, this);
        M_Only(ConnectManager)->resetClient(ClientType_Route, mIndex);

        if (mPrimaryConnect == this)
        {
            checkPrimary();
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::onTimer(TimeDurMS tick)
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
            Mlog("conn to route server timeout ");
            stop();
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            break;
        case SBID_ServerKickObject:
            prcServerKickObject(temp);
            break;
        case MSID_BubbyLoginStateNotify:
            prcBubbyLoginStateNotify(temp);
            break;
        case MSID_BuddyObjectStateA:
            prcBuddyObjectStateA(temp);
            break;
        case MSID_ReadNotify:
            prcReadNotify(temp);
            break;
        case MSID_Data:
            prcMsgData(temp);
            break;
        case SBID_P2P:
            prcP2PMessage(temp);
            break;
        case SBID_LoginStateNotify:
            prcLoginStateNotify(temp);
            break;
        case MSID_BuddyRemoveSessionS:
            prcBuddyRemoveSessionS(temp);
            break;
        case MSID_BuddyObjectStateS:
            prcBuddyObjectStateS(temp);
        case MSID_GroupMemberNotify:
            M_Only(GroupManager)->prcGroupMemberNotify(temp);
            break;
        case HSID_FileNotify:
            M_Only(FileManager)->prcFileNotify(temp);
            break;
        default:
            Mlog("unknown cmd id=%d ", temp->getCommandID());
            break;
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcServerKickObject(MdfMessage * msg)
    {
        MBCAF::ServerBase::ServerKick proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 ctype = proto.client_type();
        Mui32 reason = proto.reason();
        Mlog("kickUser, user_id=%u, client_type=%u, reason=%u. ", userid, ctype, reason);

        User * tempusr = M_Only(UserManager)->getUser(userid);
        if (tempusr)
        {
            tempusr->kickSameClientOut(ctype, reason);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcBubbyLoginStateNotify(MdfMessage * msg)
    {
        MBCAF::MsgServer::BubbyLoginStateNotify proto;
        if(!msg->toProto(&proto))
            return;

        MBCAF::Proto::ObjectState state = proto.user_stat();

        Mlog("HandleFriendStatusNotify, user_id=%u, state=%u ", state.user_id(), state.status());

        M_Only(UserManager)->broadcast(msg, CLIENT_TYPE_FLAG_PC);
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcMsgData(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if(!msg->toProto(&proto))
            return;

        if (M_GroupMessageCheck(proto.msg_type()))
        {
            M_Only(GroupManager)->prcGroupMessageBroadcast(msg);
            return;
        }

        Mui32 fromid = proto.from_user_id();
        Mui32 userid = proto.to_session_id();
        Mui32 msgid = proto.msg_id();
        Mlog("HandleMsgData, %u->%u, msg_id=%u. ", fromid, userid, msgid);

        User * fromusr = M_Only(UserManager)->getUser(fromid);
        if (fromusr)
        {
            fromusr->broadcastFromClient(msg, msgid, NULL, fromid);
        }

        User * tousr = M_Only(UserManager)->getUser(userid);
        if (tousr)
        {
            tousr->broadcastFromClient(msg, msgid, NULL, fromid);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcReadNotify(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageRead proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 req_id = proto.user_id();
        Mui32 session_id = proto.session_id();
        Mui32 msgid = proto.msg_id();
        Mui32 session_type = proto.session_type();

        Mlog("HandleMsgReadNotify, user_id=%u, session_id=%u, session_type=%u, msg_id=%u. ", req_id, session_id, session_type, msgid);
        User * tempusr = M_Only(UserManager)->getUser(req_id);
        if (tempusr)
        {
            tempusr->broadcast(msg);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcP2PMessage(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageP2P proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 fromid = proto.from_user_id();
        Mui32 userid = proto.to_user_id();

        Mlog("HandleP2PMsg, %u->%u ", fromid, userid);

        User * fromusr = M_Only(UserManager)->getUser(fromid);
        User * tousr = M_Only(UserManager)->getUser(userid);

        if (fromusr)
        {
            fromusr->broadcast(msg);
        }

        if (tousr)
        {
            tousr->broadcast(msg);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcBuddyObjectStateA(MdfMessage * msg)
    {
        MBCAF::MsgServer::BuddyObjectStateA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 result_count = proto.user_stat_list_size();
        Mlog("HandleUsersStatusResp, user_id=%u, query_count=%u ", usrid, result_count);

        MessageExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        if (extdata.getType() == MEDT_Handle)
        {
            Mui32 handle = extdata.getHandle();
            ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, handle);
            if (conn)
            {
                proto.clear_attach_data();
                msg->setProto(&proto);
                conn->send(msg);
            }
        }
        else if (extdata.getType() == MEDT_ProtoToPush)
        {
            MBCAF::Proto::ObjectState usrstate = proto.user_stat_list(0);
            MBCAF::ServerBase::UserPushQ proto2;
            if(!proto2.ParseFromArray(extdata.getProtoData(), extdata.getProtoSize()))
                return;
            MBCAF::Proto::UserTokenInfo* user_token = proto2.mutable_user_token_list(0);

            if (usrstate.status() == MBCAF::Proto::OST_Online)
            {
                user_token->set_push_type(IM_PUSH_TYPE_SILENT);
                Mlog("HandleUsersStatusResponse, user id: %d, push type: normal. ", usrstate.user_id());
            }
            else
            {
                user_token->set_push_type(IM_PUSH_TYPE_NORMAL);
                Mlog("HandleUsersStatusResponse, user id: %d, push type: normal. ", usrstate.user_id());
            }
            PushClientConnect * pushconn = PushClientConnect::getPrimaryConnect();
            if (pushconn)
            {
                MdfMessage remsg;
                remsg.setProto(&proto2);
                remsg.setCommandID(SBMSG(UserPushQ));

                pushconn->send(&remsg);
            }
        }
        else if (extdata.getType() == MEDT_ProtoToFile)
        {
            MBCAF::Proto::ObjectState usrstate = proto.user_stat_list(0);
            MBCAF::HubServer::FileTransferQ proto3;
            if(!proto3.ParseFromArray(extdata.getProtoData(), extdata.getProtoSize()))
                return;

            Mui32 handle = extdata.getHandle();

            MBCAF::Proto::TransferFileType trans_mode = MBCAF::Proto::TFT_Offline;
            if (usrstate.status() == MBCAF::Proto::OST_Online)
            {
                trans_mode = MBCAF::Proto::TFT_Online;
            }
            proto3.set_trans_mode(trans_mode);

            FileClientConnect * fileconn = FileClientConnect::getRandomConnect();
            if (fileconn)
            {
                MdfMessage remsg;
                remsg.setProto(&proto3);
                remsg.setCommandID(SBMSG(FileTransferQ));
                remsg.setSeqIdx(msg->getSeqIdx());
                fileconn->send(&remsg);
            }
            else
            {
                ServerConnect * pMsgConn = M_Only(UserManager)->getMsgConnect(proto3.from_user_id(), handle);
                if (pMsgConn)
                {
                    Mlog("no file server ");
                    MBCAF::HubServer::FileA msg4;
                    msg4.set_result_code(1);
                    msg4.set_from_user_id(proto3.from_user_id());
                    msg4.set_to_user_id(proto3.to_user_id());
                    msg4.set_file_name(proto3.fname());
                    msg4.set_task_id("");
                    msg4.set_trans_mode(proto3.trans_mode());
                    MdfMessage remsg;
                    remsg.setProto(&msg4);
                    remsg.setCommandID(HSMSG(FileA));
                    remsg.setSeqIdx(msg->getSeqIdx());

                    pMsgConn->send(&remsg);
                }
            }
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcBuddyRemoveSessionS(MdfMessage * msg)
    {
        MBCAF::MsgServer::RemoveSessionNotify proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 session_id = proto.session_id();
        Mlog("HandleRemoveSessionNotify, user_id=%u, session_id=%u ", usrid, session_id);
        User* tempusr = M_Only(UserManager)->getUser(usrid);
        if (tempusr)
        {
            tempusr->broadcast(msg);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcLoginStateNotify(MdfMessage * msg)
    {
        MBCAF::ServerBase::LoginStateNotify proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 login_status = proto.login_status();
        Mlog("HandlePCLoginStatusNotify, user_id=%u, login_status=%u ", usrid, login_status);

        User * tempusr = M_Only(UserManager)->getUser(usrid);
        if (tempusr)
        {
            tempusr->setPCState(login_status);
            MBCAF::MsgServer::PCLoginStateNotify proto2;
            proto2.set_user_id(usrid);
            if (PL_StateOn == login_status)
            {
                proto2.set_login_stat(::MBCAF::Proto::OST_Online);
            }
            else
            {
                proto2.set_login_stat(::MBCAF::Proto::OST_Offline);
            }
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(BuddyPCLoginStateNotify));
            tempusr->broadcastToMobile(&remsg);
        }
    }
    //------------------------------------------------------------------------
    void RouteClientConnect::prcBuddyObjectStateS(MdfMessage * msg)
    {
        MBCAF::MsgServer::SignInfoNotifyS proto;
        if(!msg->toProto(&proto))
            return;

        Mlog("HandleSignInfoChangedNotify, changed_user_id=%u, sign_info=%s ", proto.changed_user_id(), proto.sign_info().c_str());

        M_Only(UserManager)->broadcast(msg, CLIENT_TYPE_FLAG_BOTH);
    }
    //------------------------------------------------------------------------
    RouteClientConnect * RouteClientConnect::getPrimaryConnect()
    {
        return mPrimaryConnect;
    }
    //------------------------------------------------------------------------
}