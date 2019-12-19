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

#include "MdfDatabaseClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfPushClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfConnect.h"
#include "MdfGroupManager.h"
#include "MdfFileManager.h"
#include "security.h"
#include "MdfServerConnect.h"
#include "MdfEncrypt.h"
#include "json/json.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.MsgServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //------------------------------------------------------------------------
    static Mui32 mConnectPreServer = 0;
    static ClientReConnect * gDatabaseTimer = 0;
    static AES * gAES = 0;
    //------------------------------------------------------------------------
    void setupDataBaseConnect(const ConnectInfoList & clist, Mui32 connectcnt, const String & encryptstr)
    {
        if (clist.size() == 0)
            return;
        gAES = new AES(encryptstr);

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

        mConnectPreServer = (clist.size() / connectcnt / 2) * connectcnt;
    }
    //-----------------------------------------------------------------------
    void shutdownDataBaseConnect()
    {
        if (gDatabaseTimer)
        {
            delete gDatabaseTimer;
            gDatabaseTimer = 0;
        }
        if (gAES)
        {
            delete gAES;
            gAES = 0;
        }
    }
    //------------------------------------------------------------------------
    static DataBaseClientConnect * getClientConnectRange(Mui32 begin, Mui32 end)
    {
        srand(time(NULL));

        DataBaseClientConnect * re = 0;
        const ConnectManager::ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_DataBase);
        ConnectManager::ClientList::iterator i = clist.begin();
        ConnectManager::ClientList::iterator iend = clist.begin();
        std::advance(i, begin);
        std::advance(iend, end);
        for (; i != end; ++i)
        {
            ConnectInfo * temp = i->second;
            if (temp->mConnectState)
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
            if (temp->mConnectState)
            {
                re = static_cast<DataBaseClientConnect *>(temp->mConnect->getBase());
                break;
            }
        }

        return re;
    }
    //------------------------------------------------------------------------
    DataBaseClientConnect * DataBaseClientConnect::getSlaveConnect()
    {
        DataBaseClientConnect * re = getClientConnectRange(0, mConnectPreServer);
        if (!re) 
        {
            re = getClientConnectRange(mConnectPreServer, 
                M_Only(ConnectManager)->getClientCount(ClientType_DataBase));
        }
        return re;
    }
    //------------------------------------------------------------------------
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
    //------------------------------------------------------------------------
    DataBaseClientConnect::DataBaseClientConnect(Mui32 idx):
        mIndex(idx)
    {
    }
    //------------------------------------------------------------------------
    DataBaseClientConnect::~DataBaseClientConnect()
    {
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::connect(const String & ip, Mui16 port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        M_Only(ConnectManager)->addClientConnect(ClientType_DataBase, this);
        M_Only(ConnectManager)->confirmClient(ClientType_DataBase, mIndex);
        setTimer(true, 0, 1000);
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::onClose()
    {
        M_Only(ConnectManager)->removeClientConnect(ClientType_DataBase, this);
        M_Only(ConnectManager)->resetClient(ClientType_DataBase, mIndex);
    }
    //------------------------------------------------------------------------
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
    //------------------------------------------------------------------------
    void DataBaseClientConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID()) 
        {
        case SBID_Heartbeat:
            break;
        case SBID_UserLoginValidA:
            prcUserLoginValidA(temp);
            break;
        case SBID_TrayMsgA:
            prcTrayMsgA(temp);
            break;
        case SBID_PushShieldSA:
            prcPushShieldSA(temp);
            break;
        case SBID_PushShieldA:
            prcPushShieldA(temp);
            break;
        case MSID_UnReadCountA:
            prcUnReadCountA(temp);
            break;
        case MSID_MsgListA:
            prcMsgListA(temp);
            break;
        case MSID_MsgA:
            prcMsgA(temp);
            break;
        case MSID_Data:
            prcMsgData(temp);
            break;
        case MSID_RecentMsgA:
            prcRecentMsgA(temp);
            break;
        case MSID_BubbyRecentSessionA:
            prcBubbyRecentSessionA(temp);
            break;
        case MSID_BuddyObjectListA:
            prcBuddyObjectListA(temp);
            break;
        case MSID_BubbyObjectInfoA:
            prcBubbyObjectInfoA(temp);
            break;
        case MSID_BubbyRemoveSessionA:
            prcBubbyRemoveSessionA(temp);
            break;
        case MSID_BuddyAvatarA:
            prcBuddyAvatarA(temp);
            break;
        case MSID_BuddyChangeSignatureA:
            prcBuddyChangeSignatureA(temp);
            break;
        case MSID_BuddyOrganizationA:
            prcBuddyOrganizationA(temp);
            break;
        case SBID_SwitchTrayMsgA:
            prcSwitchTrayMsgA(temp);
            break;
        case SBID_StopReceive:
            prcStopReceive(temp);
            break;
        case SBID_GroupShieldA:
            M_Only(GroupManager)->prcGroupShieldA(temp);
            break;
        case MSID_GroupListA:
            M_Only(GroupManager)->prcGroupListA(temp);
            break;
        case MSID_GroupInfoA:
            M_Only(GroupManager)->prcGroupInfoA(temp);
            break;
        case MSID_CreateGroupA:
            M_Only(GroupManager)->prcCreateGroupA(temp);
            break;
        case MSID_GroupMemberSA:
            M_Only(GroupManager)->prcGroupMemberSA(temp);
            break;
        case MSID_GroupShieldSA:
            M_Only(GroupManager)->prcGroupShieldSA(temp);
            break;
        case HSID_OfflineFileA:
            M_Only(FileManager)->prcOfflineFileA(temp);
            break;
        default:
            Mlog("db server, wrong cmd id=%d ", temp->getCommandID());
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcUserLoginValidA(MdfMessage * msg)
    {
        MBCAF::ServerBase::UserLoginValidA proto;
        if(!msg->toProto(&proto))
            return;
        String username = proto.user_name();
        Mui32 result = proto.result_code();
        String restr = proto.result_string();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mlog("prcUserLoginValidA, user_name=%s, result=%d", username.c_str(), result);
        
        User * userobj = M_Only(UserManager)->getUser(username);
        ServerConnect * conn;
        if (!userobj) 
        {
            Mlog("MBCAF_User for user_name=%s not exist", username.c_str());
            return;
        } 
        else 
        {
            conn = userobj->getUnvalidMsgConnect(extdata.getHandle());
            if (!conn || conn->isOpen()) 
            {
                Mlog("no such conn is validated, user_name=%s", username.c_str());
                return;
            }
        }
        
        if (result != 0) 
        {
            result = MBCAF::Proto::RT_NoValidateFail;
        }
        
        if (result == 0)
        {
            MBCAF::Proto::UserInfo user_info = proto.user_info();
            Mui32 userid = user_info.user_id();
            User * tempusr = M_Only(UserManager)->getUser(userid);
            if (tempusr)
            {
                tempusr->addUnvalidMsgConnect(conn);
                userobj->removeUnvalidMsgConnect(conn);
                if (userobj->GetMsgConnMap().size() == 0)
                {
                    M_Only(UserManager)->removeUser(username);
                    delete userobj;
                }
            }
            else
            {
                tempusr = userobj;
            }
            
            tempusr->setID(userid);
            tempusr->setNick(user_info.user_nick_name());
            tempusr->setValid(true);
            M_Only(UserManager)->addUser(userid, tempusr);
            
            tempusr->kickSameClientOut(conn->getClientType(), MBCAF::Proto::KRT_Repeat, conn);
            
            RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
            if (routeconn) 
            {
                MBCAF::ServerBase::ServerKick proto2;
                proto2.set_user_id(userid);
                proto2.set_client_type((::MBCAF::Proto::ClientType)conn->getClientType());
                proto2.set_reason(1);
                MdfMessage remsg;
                remsg.setProto(&proto2);
                remsg.setCommandID(SBMSG(ServerKickObject));
                routeconn->send(&remsg);
            }
            
            Mlog("user_name: %s, uid: %d", username.c_str(), userid);
            conn->setID(userid);
            conn->setOpen(true);
            conn->SendUserStatusUpdate(MBCAF::Proto::OST_Online);
            tempusr->addMsgConnect(conn->getID(), conn);
            
            MBCAF::ServerBase::LoginA proto3;
            proto3.set_server_time(time(NULL));
            proto3.set_result_code(MBCAF::Proto::RT_OK);
            proto3.set_result_string(restr);
            proto3.set_online_status((MBCAF::Proto::ObjectStateType)conn->getOnlineState());
            MBCAF::Proto::UserInfo* user_info_tmp = proto3.mutable_user_info();
            user_info_tmp->set_user_id(user_info.user_id());
            user_info_tmp->set_user_gender(user_info.user_gender());
            user_info_tmp->set_user_nick_name(user_info.user_nick_name());
            user_info_tmp->set_avatar_url(user_info.avatar_url());
            user_info_tmp->set_sign_info(user_info.sign_info());
            user_info_tmp->set_department_id(user_info.department_id());
            user_info_tmp->set_email(user_info.email());
            user_info_tmp->set_user_real_name(user_info.user_real_name());
            user_info_tmp->set_user_tel(user_info.user_tel());
            user_info_tmp->set_user_domain(user_info.user_domain());
            user_info_tmp->set_status(user_info.status());
            MdfMessage remsg2;
            remsg2.setProto(&proto3);
            remsg2.setCommandID(SBMSG(ObjectLoginA));
            remsg2.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg2);
        }
        else
        {
            MBCAF::ServerBase::LoginA msg4;
            msg4.set_server_time(time(NULL));
            msg4.set_result_code((MBCAF::Proto::ResultType)result);
            msg4.set_result_string(restr);
            MdfMessage remsg3;
            remsg3.setProto(&msg4);
            remsg3.setCommandID(SBMSG(ObjectLoginA));
            remsg3.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg3);
            conn->stop();
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBubbyRecentSessionA(MdfMessage * msg)
    {
        MBCAF::MsgServer::RecentSessionA proto;
        if(!msg->toProto(&proto))
            return;
        Mui32 userid = proto.user_id();
        Mui32 session_cnt = proto.contact_session_list_size();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcBubbyRecentSessionA, userId=%u, session_cnt=%u", userid, session_cnt);
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen())
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBuddyObjectListA(MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryUserInfoListA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 latest_update_time = proto.latest_update_time();
        Mui32 user_cnt = proto.user_list_size();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcBuddyObjectListA, userId=%u, latest_update_time=%u, user_cnt=%u", userid, latest_update_time, user_cnt);
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen())
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcMsgListA(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageInfoListA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mui32 msg_cnt = proto.msg_list_size();
        Mui32 msg_id_begin = proto.msg_id_begin();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcMsgListA, userId=%u, session_type=%u, opposite_user_id=%u, msg_id_begin=%u, cnt=%u.", userid, session_type, session_id, msg_id_begin, msg_cnt);
        
        ServerConnect* conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcMsgA(MdfMessage *msg)
    {
        MBCAF::MsgServer::MessageInfoByIdA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mui32 msg_cnt = proto.msg_list_size();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcMsgA, userId=%u, session_type=%u, opposite_user_id=%u, cnt=%u.", userid, session_type, session_id, msg_cnt);
        
        ServerConnect* conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcMsgData(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if(!msg->toProto(&proto))
            return;
        if (M_GroupMessageCheck(proto.msg_type())) 
        {
            M_Only(GroupManager)->prcGroupMessage(msg);
            return;
        }
        
        Mui32 fromid = proto.from_user_id();
        Mui32 userid = proto.to_session_id();
        Mui32 msg_id = proto.msg_id();
        if (msg_id == 0) 
        {
            Mlog("prcMsgData, write db failed, %u->%u.", fromid, userid);
            return;
        }
        
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcMsgData, from_user_id=%u, to_user_id=%u, msg_id=%u.", fromid, userid, msg_id);
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(fromid, handle);
        if (conn)
        {
            MBCAF::MsgServer::MessageSendA proto2;
            proto2.set_user_id(fromid);
            proto2.set_msg_id(msg_id);
            proto2.set_session_id(userid);
            proto2.set_session_type(::MBCAF::Proto::ST_Single);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(DataACK));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }

        RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
        if (routeconn)
        {
            routeconn->send(msg);
        }
        
        proto.clear_attach_data();
        msg->setProto(&proto);
        User * pFromImUser = M_Only(UserManager)->getUser(fromid);
        User * tousr = M_Only(UserManager)->getUser(userid);
        msg->setSeqIdx(0);
        if (pFromImUser) 
        {
            pFromImUser->broadcastFromClient(msg, msg_id, conn, fromid);
        }

        if (tousr) 
        {
            tousr->broadcastFromClient(msg, msg_id, NULL, fromid);
        }
        
        MBCAF::ServerBase::SwitchTrayMsgQ proto3;
        proto3.add_user_id(userid);
        proto3.set_attach_data(msg->getContent(), msg->getContentSize());
        MdfMessage remsg2;
        remsg2.setProto(&proto3);
        remsg2.setCommandID(SBMSG(SwitchTrayMsgQ));
        send(&remsg2);
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcRecentMsgA(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageRecentA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 session_id = proto.session_id();
        Mui32 session_type = proto.session_type();
        Mui32 latest_msg_id = proto.latest_msg_id();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcRecentMsgA, userId=%u, session_id=%u, session_type=%u, latest_msg_id=%u.",
            userid, session_id, session_type, latest_msg_id);

        ServerConnect* conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcUnReadCountA(MdfMessage* msg)
    {
        MBCAF::MsgServer::MessageUnreadCntA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 total_cnt = proto.total_cnt();
        Mui32 user_unread_cnt = proto.unreadinfo_list_size();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcUnReadCountA, userId=%u, total_cnt=%u, user_unread_cnt=%u.", userid,
            total_cnt, user_unread_cnt);

        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBubbyObjectInfoA(MdfMessage * msg)
    {
        MBCAF::MsgServer::UserInfoListA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 user_cnt = proto.user_info_list_size();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        Mlog("prcBubbyObjectInfoA, user_id=%u, user_cnt=%u.", userid, user_cnt);
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcStopReceive(MdfMessage * msg)
    {
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBubbyRemoveSessionA(MdfMessage * msg)
    {
        MBCAF::MsgServer::RemoveSessionA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        Mui32 session_type = proto.session_type();
        Mui32 session_id = proto.session_id();
        Mlog("prcBubbyRemoveSessionA, req_id=%u, result=%u, session_id=%u, type=%u.",
            userid, result, session_id, session_type);

        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBuddyAvatarA(MdfMessage* msg)
    {
        MBCAF::MsgServer::UserAvatarSA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        
        Mlog("prcBuddyAvatarA, user_id=%u, result=%u.", userid, result);
        
        User * tempusr = M_Only(UserManager)->getUser(userid);
        if (0 != tempusr) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            tempusr->broadcast(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBuddyOrganizationA(MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryDepartListA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 latest_update_time = proto.latest_update_time();
        Mui32 dept_cnt = proto.dept_list_size();
        Mlog("prcBuddyOrganizationA, user_id=%u, latest_update_time=%u, dept_cnt=%u.", userid, latest_update_time, dept_cnt);
        
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcTrayMsgA(MdfMessage * msg)
    {
        MBCAF::ServerBase::TrayMsgA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 userid = proto.user_id();
        Mlog("prcTrayMsgA, user_id = %u.", userid);
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcSwitchTrayMsgA(MdfMessage * msg)
    {
        MBCAF::ServerBase::SwitchTrayMsgA proto;
        if(!msg->toProto(&proto))
            return;
        
        MBCAF::MsgServer::MessageData proto2;
        if(!proto2.ParseFromArray(proto.attach_data().c_str(), proto.attach_data().length()))
            return;
        String msg_data = proto2.msg_data();
        Mui32 msg_type = proto2.msg_type();
        Mui32 fromid = proto2.from_user_id();
        Mui32 toid = proto2.to_session_id();
        if (msg_type == MBCAF::Proto::MT_Text || msg_type == MBCAF::Proto::MT_GroupText)
        {
            char * msg_out = NULL;
            Mui32 msg_out_len = 0;
            if (gAES->Decrypt(msg_data.c_str(), msg_data.length(), &msg_out, msg_out_len) == 0)
            {
                msg_data = String(msg_out, msg_out_len);
            }
            else
            {
                Mlog("prcSwitchTrayMsgA, decrypt msg failed, from_id: %u, to_id: %u, msg_type: %u.", fromid, toid, msg_type);
                return;
            }
            gAES->Free(msg_out);
        }
        
        PushClientConnect::genAlter(msg_data, proto2.msg_type(), fromid);
        //{
        //    "msg_type": 1,
        //    "from_id": "1345232",
        //    "group_type": "12353",
        //}
        Json::FastWriter jwriter;
        Json::Value jsonvalue;
        jsonvalue["msg_type"] = Json::Value((Mui32)proto2.msg_type());
        jsonvalue["from_id"] = Json::Value(fromid);
        if (M_GroupMessageCheck(proto2.msg_type())) 
        {
            jsonvalue["group_id"] = Json::Value(toid);
        }
        
        Mui32 user_token_cnt = proto.user_token_info_size();
        Mlog("prcSwitchTrayMsgA, user_token_cnt = %u.", user_token_cnt);
        
        MBCAF::ServerBase::UserPushQ proto3;
        for (Mui32 i = 0; i < user_token_cnt; ++i)
        {
            MBCAF::Proto::UserTokenInfo user_token = proto.user_token_info(i);
            Mui32 userid = user_token.user_id();
            String device_token = user_token.token();
            Mui32 push_cnt = user_token.push_count();
            Mui32 ctype = user_token.user_type();

            if (fromid == userid) 
            {
                continue;
            }
            
            Mlog("prcSwitchTrayMsgA, user_id = %u, device_token = %s, push_cnt = %u, client_type = %u.",
                userid, device_token.c_str(), push_cnt, ctype);
            
            User * tempusr = M_Only(UserManager)->getUser(userid);
            if (tempusr)
            {
                proto3.set_flash(msg_data);
                proto3.set_data(jwriter.write(jsonvalue));
                MBCAF::Proto::UserTokenInfo * user_token_tmp = proto3.add_user_token_list();
                user_token_tmp->set_user_id(userid);
                user_token_tmp->set_user_type((MBCAF::Proto::ClientType)ctype);
                user_token_tmp->set_token(device_token);
                user_token_tmp->set_push_count(push_cnt);

                if (tempusr->getPCState() == PL_StateOn)
                {
                    user_token_tmp->set_push_type(IM_PUSH_TYPE_SILENT);
                    Mlog("prcSwitchTrayMsgA, user id: %d, push type: silent.", userid);
                }
                else
                {
                    user_token_tmp->set_push_type(IM_PUSH_TYPE_NORMAL);
                    Mlog("prcSwitchTrayMsgA, user id: %d, push type: normal.", userid);
                }
            }
            else
            {
                RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
                if (routeconn)
                {
                    MBCAF::ServerBase::UserPushQ msg4;
                    msg4.set_flash(msg_data);
                    msg4.set_data(jwriter.write(jsonvalue));
                    MBCAF::Proto::UserTokenInfo * user_token_tmp = msg4.add_user_token_list();
                    user_token_tmp->set_user_id(userid);
                    user_token_tmp->set_user_type((MBCAF::Proto::ClientType)ctype);
                    user_token_tmp->set_token(device_token);
                    user_token_tmp->set_push_count(push_cnt);
                    user_token_tmp->set_push_type(IM_PUSH_TYPE_NORMAL);
                    MdfMessage remsg;
                    remsg.setProto(&msg4);
                    remsg.setCommandID(SBMSG(UserPushQ));
                
                    MessageExtData extdata(MEDT_ProtoToPush, 0, remsg.getContentSize(), remsg.getContent());
                    MBCAF::MsgServer::BuddyObjectStateQ msg5;
                    msg5.set_user_id(0);
                    msg5.add_user_id_list(userid);//需要修改
                    msg5.set_attach_data(extdata.getBuffer(), extdata.getSize());
                    MdfMessage remsg2;
                    remsg2.setProto(&msg5);
                    remsg2.setCommandID(MSMSG(BuddyObjectStateQ));

                    routeconn->send(&remsg2);
                }
            }
        }

        if (proto3.user_token_list_size() > 0)
        {
            PushClientConnect * pushconn = PushClientConnect::getPrimaryConnect();
            if (pushconn) 
            {
                MdfMessage remsg3;
                remsg3.setProto(&proto3);
                remsg3.setCommandID(SBMSG(UserPushQ));

                pushconn->send(&remsg3);
            }
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcBuddyChangeSignatureA(MdfMessage * msg) 
    {
        MBCAF::MsgServer::SignInfoSA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        
        Mlog("prcBuddyChangeSignatureA: user_id=%u, result=%u.", userid, result);
        
        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
        else 
        {
            Mlog("prcBuddyChangeSignatureA: can't found msg_conn by user_id = %u, handle = %u", userid, handle);
        }
        
        if (!result) 
        {
            RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
            if(routeconn) 
            {
                MBCAF::MsgServer::SignInfoNotifyS proto2;
                proto2.set_changed_user_id(userid);
                proto2.set_sign_info(proto.sign_info());
                
                MdfMessage resmsg;
                resmsg.setProto(&proto2);
                resmsg.setCommandID(MSMSG(BuddyObjectStateS));
                
                routeconn->send(&resmsg);
            }
            else 
            {
                Mlog("prcBuddyChangeSignatureA: can't found routeconn");
            }
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcPushShieldSA(MdfMessage * msg) 
    {
        MBCAF::ServerBase::PushShieldSA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        
        Mlog("prcPushShieldSA: user_id=%u, result=%u.", userid, result);
        
        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        } 
        else 
        {
            Mlog("prcPushShieldSA: can't found msg_conn by user_id = %u, handle = %u", userid, handle);
        }
    }
    //------------------------------------------------------------------------
    void DataBaseClientConnect::prcPushShieldA(MdfMessage * msg) 
    {
        MBCAF::ServerBase::PushShieldA proto;
        if(!msg->toProto(&proto))
            return;
        
        Mui32 userid = proto.user_id();
        Mui32 result = proto.result_code();
        
        Mlog("prcPushShieldA: user_id=%u, result=%u.", userid, result);
        
        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        Mui32 handle = extdata.getHandle();
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, handle);
        if (conn && conn->isOpen()) 
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        } 
        else 
        {
            Mlog("prcPushShieldA: can't found msg_conn by user_id = %u, handle = %u", userid, handle);
        }
    }
    //------------------------------------------------------------------------
}