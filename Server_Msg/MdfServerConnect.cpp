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
#include "MdfDatabaseClientConnect.h"
#include "MdfLoginClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfConnectManager.h"
#include "MdfFileManager.h"
#include "MdfGroupManager.h"
#include "MdfUserManager.h"
#include "MdfServerConnect.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    #define LOGIN_A_TimeOut             15000
    #define MSG_DataAck_TimeOut         15000
    #define MSG_StatLog_Interval        300000
    #define MaxMessagePerSecond         20
    static Mui64 gLastStatTick;
    static Mui32 gDownMsgTotalCnt = 0;
    static Mui32 gDownMsgMissCnt = 0;
    static bool gLogMsg = true;
    //-----------------------------------------------------------------------
    class ServerConnectTimer : public ConnectManager::TimerListener
    {
    protected:
        /// @copydetails TimerListener::onTimer
        void onTimer(TimeDurMS tick)
        {
            if (tick > gLastStatTick + MSG_StatLog_Interval)
            {
                gLastStatTick = cur_time;
                Mlog("down_msg_cnt=%u, down_msg_miss_cnt=%u ", gDownMsgTotalCnt, gDownMsgMissCnt);
            }
        }
    }
    //-----------------------------------------------------------------------
    static void signalUsr1(int sig_no)
    {
        if (sig_no == SIGUSR1)
        {
            Mlog("receive SIGUSR1 ");
            gDownMsgTotalCnt = 0;
            gDownMsgMissCnt = 0;
        }
    }
    //-----------------------------------------------------------------------
    static void signalUsr2(int sig_no)
    {
        if (sig_no == SIGUSR2)
        {
            Mlog("receive SIGUSR2 ");
            gLogMsg = !gLogMsg;
        }
    }
    //-----------------------------------------------------------------------
    static void signalHup(int sig_no)
    {
        if (sig_no == SIGHUP)
        {
            Mlog("receive SIGHUP exit... ");
            ACE_OS::exit(0);
        }
    }
    //-----------------------------------------------------------------------
    void setupSignal()
    {
        gLastStatTick = M_Only(ConnectManager)->getTimeTick();
        signal(SIGUSR1, signalUsr1);
        signal(SIGUSR2, signalUsr2);
        signal(SIGHUP, signalHup);
    }
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect()
    {
        mUserID = 0;
        mOpen = false;
        mKick = false;
        mMsgPerSecond = 0;
        mOnlineState = MBCAF::Proto::OST_Offline;
    }
    //-----------------------------------------------------------------------
    ServerConnect::~ServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        M_Only(ConnectManager)->addServerConnect(ServerType_Server, this);
        mLoginTime = M_Only(ConnectManager)->getTimeTick();
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        M_Only(ConnectManager)->removeServerConnect(ServerType_Server, this);
        Mlog("Close client, handle=%d, user_id=%u ", getID, mUserID);

        User * userobj = M_Only(UserManager)->getUser(mUserID);
        if (userobj)
        {
            userobj->removeMsgConnect(m_handle);
            userobj->removeUnvalidMsgConnect(this);

            SendUserStatusUpdate(::MBCAF::Proto::OST_Offline);
            if (userobj->GetMsgConnMap().size() == 0)
            {
                M_Only(UserManager)->destroyUser(userobj);
            }
        }

        userobj = M_Only(UserManager)->getUser(mLoginID);
        if (userobj)
        {
            userobj->removeUnvalidMsgConnect(this);
            if (userobj->GetMsgConnMap().size() == 0)
            {
                M_Only(UserManager)->destroyUser(userobj);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS tick)
    {
        mMsgPerSecond = 0;

        if (M_MobileLoginCheck(mClientType))
        {
            if (tick > mReceiveMark + M_MobileClient_Timeout)
            {
                Mlog("mobile client timeout, handle=%d, uid=%u ", m_handle, mUserID);
                stop();
                return;
            }
        }
        else
        {
            if (tick > mReceiveMark + M_Client_Timeout)
            {
                Mlog("client timeout, handle=%d, uid=%u ", m_handle, mUserID);
                stop();
                return;
            }
        }

        if (!isOpen())
        {
            if (tick > mLoginTime + LOGIN_A_TimeOut)
            {
                Mlog("login timeout, handle=%d, uid=%u ", m_handle, mUserID);
                stop();
                return;
            }
        }

        list<MessageACK>::iterator it, itend = mSendList.end();
        for (it = mSendList.begin(); it != itend; ++it)
        {
            MessageACK msg = *it;
            list<MessageACK>::iterator it_old = it++;
            if (tick >= msg.mTimeStamp + MSG_DataAck_TimeOut)
            {
                Mlog("!!!a msg missed, msg_id=%u, %u->%u ", msg.mMessageID, msg.mFromID, mUserID);
                gDownMsgMissCnt++;
                mSendList.erase(it_old);
            }
            else
            {
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        if (temp->getCommandID() != SBID_ObjectLoginQ && !isOpen() && isKickOff())
        {
            Mlog("onMessage, wrong msg. ");
            M_EXCEPT(MessageType, "onMessage error, user not login. ", temp->getMessageType(), temp->getMessageID());
            return;
        }
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            prcHeartBeat(temp);
            break;
        case SBID_ObjectLoginQ:
            prcObjectLoginQ(temp);
            break;
        case SBID_ObjectLogoutQ:
            prcObjectLogoutQ(temp);
            break;
        case SBID_TrayMsgQ:
            prcTrayMsgQ(temp);
            break;
        case SBID_KickPCClientQ:
            prcKickPCClient(temp);
            break;
        case SBID_PushShieldSQ:
            prcPushShieldSQ(temp);
            break;
        case SBID_PushShieldQ:
            prcPushShieldQ(temp);
            break;
        case MSID_Data:
            prcMessageData(temp);
            break;
        case MSID_DataACK:
            prcMessageDataACK(temp);
            break;
        case MSID_TimeQ:
            prcTimeQ(temp);
            break;
        case MSID_MsgListQ:
            prcMessageListQ(temp);
            break;
        case MSID_MsgQ:
            prcMessageQ(temp);
            break;
        case MSID_UnReadCountQ:
            prcUnReadCountQ(temp);
            break;
        case MSID_ReadACK:
            prcReadACK(temp);
            break;
        case MSID_RecentMsgQ:
            prcRecentMessageQ(temp);
            break;
        case SBID_P2P:
            prcP2PMessage(temp);
            break;
        case MSID_BubbyRecentSessionQ:
            prcBubbyRecentSessionQ(temp);
            break;
        case MSID_BubbyObjectInfoQ:
            prcBubbyObjectInfoQ(temp);
            break;
        case MSID_BubbyRemoveSessionQ:
            prcBubbyRemoveSessionQ(temp);
            break;
        case MSID_BuddyObjectListQ:
            prcBuddyObjectListQ(temp);
            break;
        case MSID_BuddyAvatarQ:
            prcBuddyAvatarQ(temp);
            break;
        case MSID_BuddyChangeSignatureQ:
            prcBuddyChangeSignatureQ(temp);
            break;
        case MSID_BuddyObjectStateQ:
            prcBuddyObjectStateQ(temp);
            break;
        case MSID_BuddyOrganizationQ:
            prcBuddyOrganizationQ(temp);
            break;
        case MSID_GroupListQ:
            M_Only(GroupManager)->GroupListQ(this, temp);
            break;
        case MSID_GroupInfoQ:
            M_Only(GroupManager)->GroupInfoListQ(this, temp);
            break;
        case MSID_CreateGroupQ:
            M_Only(GroupManager)->GroupCreateQ(this, temp);
            break;
        case MSID_GroupMemberSQ:
            M_Only(GroupManager)->GroupMemberSQ(this, temp);
            break;
        case MSID_GroupShieldSQ:
            M_Only(GroupManager)->GroupShieldSQ(this, temp);
            break;
        case HSID_FileQ:
            M_Only(FileManager)->FileQ(this, temp);
            break;
        case HSID_OfflineFileQ:
            M_Only(FileManager)->OfflineFileQ(this, temp);
            break;
        case HSID_AddOfflineFileQ:
            M_Only(FileManager)->AddOfflineFileQ(this, temp);
            break;
        case HSID_DeleteOfflineFileQ:
            M_Only(FileManager)->DeleteOfflineFileQ(this, temp);
            break;
        default:
            Mlog("wrong msg, cmd id=%d, user id=%u. ", temp->getCommandID(), mID);
            break;
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcHeartBeat(MdfMessage * msg)
    {
        send(msg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcObjectLoginQ(MdfMessage * msg)
    {
        if(mLoginID.length() != 0)
        {
            Mlog("duplicate LoginRequest in the same conn ");
            return;
        }

        Mui32 result = 0;
        String restr = "";
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getSlaveConnect();
        if(!dbconn)
        {
            result = MBCAF::Proto::RT_NoDataBaseServer;
            restr = "server exception";
        }
        else if(!LoginClientConnect::getPrimaryConnect())
        {
            result = MBCAF::Proto::RT_NoLoginServer;
            restr = "server exception";
        }
        else if(!RouteClientConnect::getPrimaryConnect())
        {
            result = MBCAF::Proto::RT_NoRouteServer;
            restr = "server exception";
        }

        if (result)
        {
            MBCAF::ServerBase::LoginA proto;
            proto.set_server_time(time(NULL));
            proto.set_result_code((MBCAF::Proto::ResultType)result);
            proto.set_result_string(restr);
            MdfMessage remsg;
            remsg.setProto(&proto);
            remsg.setCommandID(SBMSG(ObjectLoginA));
            remsg.setSeqIdx(msg->getSeqIdx());
            send(&remsg);
            stop();
            return;
        }

        MBCAF::ServerBase::LoginQ proto;
        if(!msg->toProto(&proto))
            return;

        mLoginID = proto.user_name();
        String password = proto.password();
        Mui32 online_status = proto.online_status();
        if (online_status < MBCAF::Proto::OST_Online || online_status > MBCAF::Proto::USER_STATUS_LEAVE)
        {
            Mlog("HandleLoginReq, online state wrong: %u ", online_status);
            online_status = MBCAF::Proto::OST_Online;
        }
        mClientVersion = proto.client_version();
        mClientType = proto.client_type();
        mOnlineState = online_status;
        Mlog("HandleLoginReq, user_name=%s, state=%u, client_type=%u, client=%s, ",
            mLoginID.c_str(), online_status, mClientType, mClientVersion.c_str());
        User * userobj = M_Only(UserManager)->getUser(mLoginID);
        if (!userobj)
        {
            userobj = new User(mLoginID);
            M_Only(UserManager)->addUser(mLoginID, userobj);
        }
        userobj->addUnvalidMsgConnect(this);

        HandleExtData extdata(m_handle);
        MBCAF::ServerBase::UserLoginValidQ proto2;
        proto2.set_user_name(proto.user_name());
        proto2.set_password(password);
        proto2.set_attach_data(extdata.getBuffer(), extdata.getSize());
        MdfMessage remsg;
        remsg.setProto(&proto2);
        remsg.setCommandID(SBMSG(UserLoginValidQ));
        remsg.setSeqIdx(msg->getSeqIdx());
        dbconn->send(&remsg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcObjectLogoutQ(MdfMessage * msg)
    {
        Mlog("HandleLoginOutRequest, user_id=%d, client_type=%u. ", mID, mClientType);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            MBCAF::ServerBase::TrayMsgQ proto;
            proto.set_user_id(mID);
            proto.set_device_token("");
            MdfMessage remsg;
            remsg.setProto(&proto);
            remsg.setCommandID(SBMSG(TrayMsgQ));
            remsg.setSeqIdx(msg->getSeqIdx());
            dbconn->send(&remsg);
        }

        MBCAF::ServerBase::LogoutA proto2;
        proto2.set_result_code(0);
        MdfMessage remsg2;
        remsg2.setProto(&proto2);
        remsg2.setCommandID(SBMSG(ObjectLogoutA));
        remsg2.setSeqIdx(msg->getSeqIdx());
        send(&remsg2);
        stop();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcKickPCClient(MdfMessage * msg)
    {
        MBCAF::ServerBase::KickPCConnectQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 user_id = mID;
        if (!M_MobileLoginCheck(mClientType))
        {
            Mlog("HandleKickPCClient, user_id = %u, cmd must come from mobile client. ", user_id);
            return;
        }
        Mlog("HandleKickPCClient, user_id = %u. ", user_id);

        User * userobj = M_Only(UserManager)->getUser(user_id);
        if (userobj)
        {
            userobj->kickSameClientOut(CT_MAC, MBCAF::Proto::KRT_Mobile, this);
        }

        RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
        if (routeconn)
        {
            MBCAF::ServerBase::ServerKick proto2;
            proto2.set_user_id(user_id);
            proto2.set_client_type(::MBCAF::Proto::CT_MAC);
            proto2.set_reason(MBCAF::Proto::KRT_Mobile);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(SBMSG(ServerKickObject));
            routeconn->send(&remsg);
        }

        MBCAF::ServerBase::KickPCConnectA proto3;
        proto3.set_user_id(user_id);
        proto3.set_result_code(0);
        MdfMessage remsg;
        remsg.setProto(&proto3);
        remsg.setCommandID(SBMSG(KickPCClientA));
        remsg.setSeqIdx(msg->getSeqIdx());
        send(&remsg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBubbyRecentSessionQ(MdfMessage * msg)
    {
        DataBaseClientConnect * conn = DataBaseClientConnect::getSlaveConnect();
        if (conn)
        {
            MBCAF::MsgServer::RecentSessionQ proto;
            if(!msg->toProto(&proto))
                return;
            Mlog("HandleClientRecentContactSessionRequest, user_id=%u, latest_update_time=%u. ", mID, proto.latest_update_time());

            proto.set_user_id(mID);

            HandleExtData extdata(m_handle);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcMessageData(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if(!msg->toProto(&proto))
            return;
        if (proto.msg_data().length() == 0)
        {
            Mlog("discard an empty message, uid=%u ", mID);
            return;
        }

        if (mMsgPerSecond >= MaxMessagePerSecond)
        {
            Mlog("!!!too much msg cnt in one second, uid=%u ", mID);
            return;
        }

        if (proto.from_user_id() == proto.to_session_id() && M_SingleMessageCheck(proto.msg_type()))
        {
            Mlog("!!!from_user_id == to_user_id. ");
            return;
        }

        ++mMsgPerSecond;

        Mui32 seid = proto.to_session_id();
        Mui32 msgid = proto.msg_id();
        uint8_t msg_type = proto.msg_type();
        String msg_data = proto.msg_data();

        if (gLogMsg)
        {
            Mlog("HandleClientMsgData, %d->%d, msg_type=%u, msg_id=%u. ", mID, seid, msg_type, msgid);
        }

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            Mui32 ctime = time(NULL);
            HandleExtData extdata(m_handle);
            proto.set_from_user_id(mUserID);
            proto.set_create_time(ctime);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);

            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcMessageDataACK(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageSendA proto;
        if(!msg->toProto(&proto))
            return;

        MBCAF::Proto::SessionType session_type = proto.session_type();
        if (session_type == MBCAF::Proto::ST_Single)
        {
            Mui32 msgid = proto.msg_id();
            Mui32 session_id = proto.session_id();
            removeSend(msgid, session_id);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcTimeQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::ClientTimeA proto;
        msg.set_server_time((Mui32)time(NULL));
        MdfMessage remsg;
        remsg.setProto(&proto);
        remsg.setCommandID(MSMSG(TimeA));
        remsg.setSeqIdx(msg->getSeqIdx());
        send(&remsg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcMessageListQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageInfoListQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 session_id = proto.session_id();
        Mui32 msg_id_begin = proto.msg_id_begin();
        Mui32 msg_cnt = proto.msg_cnt();
        Mui32 session_type = proto.session_type();
        Mlog("HandleClientGetMsgListRequest, req_id=%u, session_type=%u, session_id=%u, msg_id_begin=%u, msg_cnt=%u. ",
            mUserID, session_type, session_id, msg_id_begin, msg_cnt);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getSlaveConnect();
        if (dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcMessageQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageInfoByIdQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 session_id = proto.session_id();
        Mui32 session_type = proto.session_type();
        Mui32 msg_cnt = proto.msg_id_list_size();
        Mlog("prcMessageQ, req_id=%u, session_type=%u, session_id=%u, msg_cnt=%u.",
            mUserID, session_type, session_id, msg_cnt);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getSlaveConnect();
        if (dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcUnReadCountQ(MdfMessage * msg)
    {
        Mlog("HandleClientUnreadMsgCntReq, from_id=%u ", mUserID);
        MBCAF::MsgServer::MessageUnreadCntQ proto;
        if(!msg->toProto(&proto))
            return;

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getSlaveConnect();
        if (dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcReadACK(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageReadA proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mui32 msgid = proto.msg_id();
        Mlog("HandleClientMsgReadAck, user_id=%u, session_id=%u, msg_id=%u, session_type=%u. ", mUserID, session_id, msgid, session_type);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(mUserID);
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        MBCAF::MsgServer::MessageRead proto2;
        proto2.set_user_id(mUserID);
        proto2.set_session_id(session_id);
        proto2.set_msg_id(msgid);
        proto2.set_session_type((MBCAF::Proto::SessionType)session_type);
        MdfMessage remsg;
        remsg.setProto(&proto2);
        remsg.setCommandID(MSMSG(ReadNotify));
        User * tempusr = M_Only(UserManager)->getUser(mUserID);
        if (tempusr)
        {
            tempusr->broadcast(&remsg, this);
        }
        RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
        if (routeconn)
        {
            routeconn->send(&remsg);
        }

        if (session_type == MBCAF::Proto::ST_Single)
        {
            removeSend(msgid, session_id);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcRecentMessageQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageRecentQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mlog("HandleClientGetMsgListRequest, user_id=%u, session_id=%u, session_type=%u. ", mUserID, session_id, session_type);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcP2PMessage(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageP2P proto;
        if(!msg->toProto(&proto))
            return;
        String cmd_msg = proto.cmd_msg_data();
        Mui32 from_user_id = proto.from_user_id();
        Mui32 userid = proto.to_user_id();

        Mlog("HandleClientP2PCmdMsg, %u->%u, cmd_msg: %s ", from_user_id, userid, cmd_msg.c_str());

        User * fromusr = M_Only(UserManager)->getUser(mUserID);
        User * tousr = M_Only(UserManager)->getUser(userid);

        if (fromusr)
        {
            fromusr->broadcast(msg, this);
        }

        if (tousr)
        {
            tousr->broadcast(msg, NULL);
        }

        RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
        if (routeconn)
        {
            routeconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBubbyObjectInfoQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::UserInfoListQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 user_cnt = proto.user_id_list_size();
        Mlog("HandleClientUserInfoReq, req_id=%u, user_cnt=%u ", mUserID, user_cnt);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getSlaveConnect();
        if(dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBubbyRemoveSessionQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::RemoveSessionQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mlog("HandleClientRemoveSessionReq, user_id=%u, session_id=%u, type=%u ", mUserID, session_id, session_type);

        DataBaseClientConnect * conn = DataBaseClientConnect::getPrimaryConnect();
        if (conn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            conn->send(msg);
        }

        if (session_type == MBCAF::Proto::ST_Single)
        {
            MBCAF::MsgServer::RemoveSessionNotify proto2;
            proto2.set_user_id(mUserID);
            proto2.set_session_id(session_id);
            proto2.set_session_type((MBCAF::Proto::SessionType)session_type);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(BuddyRemoveSessionS));
            User * userobj = M_Only(UserManager)->getUser(mUserID);
            if (userobj)
            {
                userobj->broadcast(&remsg, this);
            }
            RouteClientConnect* routeconn = RouteClientConnect::getPrimaryConnect();
            if (routeconn)
            {
                routeconn->send(&remsg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBuddyObjectListQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryUserInfoListQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 latest_update_time = proto.latest_update_time();
        Mlog("HandleClientAllUserReq, user_id=%u, latest_update_time=%u. ", mUserID, latest_update_time);

        DataBaseClientConnect * conn = DataBaseClientConnect::getPrimaryConnect();
        if (conn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBuddyAvatarQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::UserAvatarSQ proto;
        if(!msg->toProto(&proto))
            return;
        Mlog("HandleChangeAvatarRequest, user_id=%u ", mUserID);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(mUserID);
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBuddyObjectStateQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::BuddyObjectStateQ proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 user_count = proto.user_id_list_size();
        Mlog("HandleClientUsersStatusReq, user_id=%u, query_count=%u.", mUserID, user_count);

        RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
        if (routeconn)
        {
            proto.set_user_id(mUserID);
            MessageExtData attach(MEDT_Handle, m_handle, 0, NULL);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            routeconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBuddyOrganizationQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryDepartListQ proto;
        if(!msg->toProto(&proto))
            return;
        Mlog("HandleClientDepartmentRequest, user_id=%u, latest_update_time=%u.", mUserID, proto.latest_update_time());
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            HandleExtData attach(m_handle);
            proto.set_user_id(mUserID);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcTrayMsgQ(MdfMessage * msg)
    {
        if (!M_MobileLoginCheck(getClientType()))
        {
            Mlog("HandleClientDeviceToken, user_id=%u, not mobile client.", mUserID);
            return;
        }
        MBCAF::ServerBase::TrayMsgQ proto;
        if(!msg->toProto(&proto))
            return;
        String device_token = proto.device_token();
        Mlog("HandleClientDeviceToken, user_id=%u, device_token=%s ", mUserID, device_token.c_str());

        MBCAF::ServerBase::TrayMsgA proto2;
        proto2.set_user_id(mUserID);
        MdfMessage remsg;
        remsg.setProto(&proto2);
        remsg.setCommandID(SBMSG(TrayMsgA));
        remsg.setSeqIdx(msg->getSeqIdx());
        send(&remsg);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(mUserID);
            proto.set_client_type((::MBCAF::Proto::ClientType)getClientType());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::addSend(Mui32 msgid, Mui32 fromid)
    {
        MessageACK ack;
        ack.mMessageID = msgid;
        ack.mFromID = fromid;
        ack.mTimeStamp = M_Only(ConnectManager)->getTimeTick();
        mSendList.push_back(ack);

        ++gDownMsgTotalCnt;
    }
    //-----------------------------------------------------------------------
    void ServerConnect::removeSend(Mui32 msgid, Mui32 fromid)
    {
        list<MessageACK>::iterator it, itend = mSendList.end();
        for (it = mSendList.begin(); it != itend; ++it)
        {
            MessageACK msg = *it;
            if ((msg.mMessageID == msgid) && (msg.mFromID == fromid))
            {
                mSendList.erase(it);
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::SendUserStatusUpdate(Mui32 state)
    {
        if (mOpen)
        {
            User * userobj = M_Only(UserManager)->getUser(mUserID);
            if (userobj)
            {

                if (state == ::MBCAF::Proto::OST_Online)
                {
                    MBCAF::ServerBase::ServerUserCount proto;
                    proto.set_user_id(mUserID);
                    proto.set_user_action(ServerConnectInc);
                    MdfMessage remsg;
                    remsg.setProto(&proto);
                    remsg.setCommandID(SBMSG(ConnectNotify));
                    M_Only(ConnectManager)->sendClientConnect(ClientType_Login, &remsg);

                    MBCAF::ServerBase::ObjectConnectState proto2;
                    proto2.set_user_id(mUserID);
                    proto2.set_user_status(::MBCAF::Proto::OST_Online);
                    proto2.set_client_type((::MBCAF::Proto::ClientType)mClientType);
                    MdfMessage remsg2;
                    remsg2.setProto(&proto2);
                    remsg2.setCommandID(SBMSG(ObjectInfoS));

                    M_Only(ConnectManager)->sendClientConnect(ClientType_Route, &remsg2);
                }
                else if (state == ::MBCAF::Proto::OST_Offline)
                {
                    MBCAF::ServerBase::ServerUserCount proto;
                    proto.set_user_id(mUserID);
                    proto.set_user_action(USER_CNT_DEC);
                    MdfMessage remsg;
                    remsg.setProto(&proto);
                    remsg.setCommandID(SBMSG(ConnectNotify));
                    M_Only(ConnectManager)->sendClientConnect(ClientType_Login, &remsg);

                    MBCAF::ServerBase::ObjectConnectState proto2;
                    proto2.set_user_id(mUserID);
                    proto2.set_user_status(::MBCAF::Proto::OST_Offline);
                    proto2.set_client_type((::MBCAF::Proto::ClientType)mClientType);
                    MdfMessage remsg2;
                    remsg2.setProto(&proto2);
                    remsg2.setCommandID(SBMSG(ObjectInfoS));

                    M_Only(ConnectManager)->sendClientConnect(ClientType_Route, &remsg2);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcBuddyChangeSignatureQ(MdfMessage * msg)
    {
        MBCAF::MsgServer::SignInfoSQ proto;
        if(!msg->toProto(&proto))
            return;
        Mlog("HandleChangeSignInfoRequest, user_id=%u ", mUserID);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if(dbconn)
        {
            proto.set_user_id(mUserID);
            MessageExtData attach(MEDT_Handle, m_handle, 0, NULL);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());

            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcPushShieldSQ(MdfMessage * msg)
    {
        MBCAF::ServerBase::PushShieldSQ proto;
        if(!msg->toProto(&proto))
            return;
        Mlog("prcPushShieldSQ, user_id=%u, shield_status ", mUserID, proto.shield_status());
        DataBaseClientConnect* dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(mUserID);
            MessageExtData attach(MEDT_Handle, m_handle, 0, NULL);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());

            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcPushShieldQ(MdfMessage * msg)
    {
        MBCAF::ServerBase::PushShieldQ proto;
        if(!msg->toProto(&proto))
            return;
        Mlog("HandleChangeSignInfoRequest, user_id=%u ", mUserID);
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(mUserID);
            MessageExtData attach(MEDT_Handle, m_handle, 0, NULL);
            proto.set_attach_data(attach.getBuffer(), attach.getSize());

            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
}