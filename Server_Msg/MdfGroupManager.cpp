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

#include "MdfGroupManager.h"
#include "MdfServerConnect.h"
#include "MdfDatabaseClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfUserManager.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    M_SingletonImpl(GroupManager);
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupListA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupListA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 gcnt = proto.group_version_list_size();
        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());

        Mlog("prcGroupListA, user_id=%u, group_cnt=%u. ", usrid, gcnt);

        proto.clear_attach_data();
        msg->setProto(&proto);
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, extdata.getHandle());
        if (conn)
        {
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupInfoA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupInfoListA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 gcnt = proto.group_info_list_size();
        MessageExtData pduAttachData((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());

        Mlog("prcGroupInfoA, user_id=%u, group_cnt=%u. ", usrid, gcnt);

        if (pduAttachData.getProtoSize() > 0 && gcnt > 0)
        {
            MBCAF::Proto::GroupInfo ginfo = proto.group_info_list(0);
            Mui32 gid = ginfo.group_id();
            Mlog("GroupInfoRequest is send by server, group_id=%u ", gid);

            std::set<Mui32> memberlist;
            for (Mui32 i = 0; i < ginfo.group_member_list_size(); ++i)
            {
                memberlist.insert(ginfo.group_member_list(i));
            }
            if (memberlist.find(usrid) == memberlist.end())
            {
                Mlog("user_id=%u is not in group, group_id=%u. ", usrid, gid);
                return;
            }

            MBCAF::MsgServer::MessageData proto2;
            if (!proto2.ParseFromArray(pduAttachData.getProtoData(), pduAttachData.getProtoSize()))
                return;
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(Data));

            MBCAF::ServerBase::GroupShieldQ proto3;
            proto3.set_group_id(gid);
            proto3.set_attach_data(remsg.getContent(), remsg.getContentSize());
            for (Mui32 i = 0; i < ginfo.group_member_list_size(); ++i)
            {
                Mui32 member_user_id = ginfo.group_member_list(i);

                proto3.add_user_id(member_user_id);

                User* tousr = M_Only(UserManager)->getUser(member_user_id);
                if (tousr)
                {
                    ServerConnect * conn = NULL;
                    if (member_user_id == usrid)
                    {
                        Mui32 reqHandle = pduAttachData.getHandle();
                        if (reqHandle != 0)
                            conn = M_Only(UserManager)->getMsgConnect(usrid, reqHandle);
                    }

                    tousr->broadcast(remsg.getBuffer(), remsg.getSize(), conn);
                }
            }

            MdfMessage pdu2;
            pdu2.setProto(&proto3);
            pdu2.setCommandID(SBMSG(GroupShieldQ));
            DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
            if (dbconn)
            {
                dbconn->send(&pdu2);
            }
        }
        else if (pduAttachData.getProtoSize() == 0)
        {
            ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, pduAttachData.getHandle());
            if (conn)
            {
                proto.clear_attach_data();
                msg->setProto(&proto);
                conn->send(msg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupMessageBroadcast(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 fromid = proto.from_user_id();
        Mui32 sessionid = proto.to_session_id();
        String msgdata = proto.msg_data();
        Mui32 msgid = proto.msg_id();
        Mlog("prcGroupMessageBroadcast, %u->%u, msg id=%u. ", fromid, sessionid, msgid);

        MessageExtData pduAttachData(MEDT_HandleProto, 0, msg->getContentSize(), msg->getContent());
        MBCAF::MsgServer::GroupInfoListQ proto2;
        proto2.set_user_id(fromid);
        MBCAF::Proto::GroupVersionInfo * group_version_info = proto2.add_group_version_list();
        group_version_info->set_group_id(sessionid);
        group_version_info->set_version(0);
        proto2.set_attach_data(pduAttachData.getBuffer(), pduAttachData.getSize());
        MdfMessage remsg;
        remsg.setProto(&proto2);
        remsg.setCommandID(MSMSG(GroupInfoQ));
        DataBaseClientConnect* dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            dbconn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcCreateGroupA(MdfMessage* msg)
    {
        MBCAF::MsgServer::GroupCreateA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 usrid = proto.user_id();
        Mui32 result = proto.result_code();
        Mui32 gid = proto.group_id();
        String gname = proto.group_name();
        Mui32 usrcnt = proto.user_id_list_size();
        Mlog("prcCreateGroupA, req_id=%u, result=%u, group_id=%u, group_name=%s, member_cnt=%u. ", usrid, result, gid, gname.c_str(), usrcnt);

        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, extdata.getHandle());
        if (conn)
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupMemberSA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupMemberSA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 change_type = proto.change_type();
        Mui32 usrid = proto.user_id();
        Mui32 result = proto.result_code();
        Mui32 gid = proto.group_id();
        Mui32 cusrcnt = proto.chg_user_id_list_size();
        Mui32 usrcnt = proto.cur_user_id_list_size();
        Mlog("HandleChangeMemberResp, change_type=%u, req_id=%u, group_id=%u, result=%u, chg_usr_cnt=%u, usrcnt=%u. ",
            change_type, usrid, gid, result, cusrcnt, usrcnt);

        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, extdata.getHandle());

        if (conn)
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }

        if (!result)
        {
            MBCAF::MsgServer::GroupMemberNotify proto2;
            proto2.set_user_id(usrid);
            proto2.set_change_type((::MBCAF::Proto::GroupModifyType)change_type);
            proto2.set_group_id(gid);
            for (Mui32 i = 0; i < cusrcnt; ++i)
            {
                proto2.add_chg_user_id_list(proto.chg_user_id_list(i));
            }
            for (Mui32 i = 0; i < usrcnt; ++i)
            {
                proto2.add_cur_user_id_list(proto.cur_user_id_list(i));
            }
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(GroupMemberNotify));
            RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
            if (routeconn)
            {
                routeconn->send(&remsg);
            }

            for (Mui32 i = 0; i < cusrcnt; ++i)
            {
                Mui32 userid = proto.chg_user_id_list(i);
                sendToUser(userid, &remsg, conn);
            }
            for (Mui32 i = 0; i < usrcnt; ++i)
            {
                Mui32 userid = proto.cur_user_id_list(i);
                sendToUser(userid, &remsg, conn);
            }
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupMemberNotify(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupMemberNotify proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 change_type = proto.change_type();
        Mui32 gid = proto.group_id();
        Mui32 cusrcnt = proto.chg_user_id_list_size();
        Mui32 usrcnt = proto.cur_user_id_list_size();
        Mlog("HandleChangeMemberBroadcast, change_type=%u, group_id=%u, chg_user_cnt=%u, usrcnt=%u. ", change_type, gid, cusrcnt, usrcnt);

        for (Mui32 i = 0; i < cusrcnt; ++i)
        {
            Mui32 userid = proto.chg_user_id_list(i);
            sendToUser(userid, msg);
        }
        for (Mui32 i = 0; i < usrcnt; ++i)
        {
            Mui32 userid = proto.cur_user_id_list(i);
            sendToUser(userid, msg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::GroupListQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupListQ proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 usrid = conn->getUserID();
        Mlog("GroupListQ, user_id=%u. ", usrid);
        HandleExtData extdata(conn->getID());

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            proto.set_user_id(usrid);
            proto.set_attach_data((Mui8 *)extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        else
        {
            Mlog("no db connection. ");
            MBCAF::MsgServer::GroupListA proto2;
            proto.set_user_id(usrid);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(GroupListA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::GroupInfoListQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupInfoListQ proto;
        if (!msg->toProto(&proto))
            return;
        Mui32 usrid = conn->getUserID();
        Mui32 gcnt = proto.group_version_list_size();
        Mlog("GroupInfoListQ, user_id=%u, group_cnt=%u. ", usrid, gcnt);
        MessageExtData extdata(MEDT_Handle, conn->getID(), 0, NULL);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if(dbconn)
        {
            proto.set_user_id(usrid);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        else
        {
            Mlog("no db connection. ");
            MBCAF::MsgServer::GroupInfoListA proto2;
            proto2.set_user_id(usrid);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(GroupInfoA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupMessage(MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if (!msg->toProto(&proto))
            return;
        Mui32 fromid = proto.from_user_id();
        Mui32 sessionid = proto.to_session_id();
        String msgdata = proto.msg_data();
        Mui32 msgid = proto.msg_id();
        if (msgid == 0)
        {
            Mlog("HandleGroupMsg, write db failed, %u->%u. ", fromid, sessionid);
            return;
        }
        uint8_t msg_type = proto.msg_type();
        HandleExtData extdata((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());

        Mlog("HandleGroupMsg, %u->%u, msg id=%u. ", fromid, sessionid, msgid);

        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(fromid, extdata.getHandle());

        if (conn)
        {
            MBCAF::MsgServer::MessageSendA proto2;
            proto2.set_user_id(fromid);
            proto2.set_session_id(sessionid);
            proto2.set_msg_id(msgid);
            proto2.set_session_type(::MBCAF::Proto::ST_Group);
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

        MessageExtData pduAttachData(MEDT_HandleProto, extdata.getHandle(), msg->getContentSize(), msg->getContent());
        MBCAF::MsgServer::GroupInfoListQ proto3;
        proto3.set_user_id(fromid);
        MBCAF::Proto::GroupVersionInfo * group_version_info = proto3.add_group_version_list();
        group_version_info->set_group_id(sessionid);
        group_version_info->set_version(0);
        proto3.set_attach_data(pduAttachData.getBuffer(), pduAttachData.getSize());
        MdfMessage remsg;
        remsg.setProto(&proto3);
        remsg.setCommandID(MSMSG(GroupInfoQ));
        DataBaseClientConnect* dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            dbconn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::GroupCreateQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupCreateQ proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 req_user_id = conn->getUserID();
        String gname = proto.group_name();
        Mui32 gtype = proto.group_type();
        if (gtype == MBCAF::Proto::GROUP_TYPE_NORMAL)
        {
            Mlog("GroupCreateQ, create normal group failed, req_id=%u, group_name=%s. ", req_user_id, gname.c_str());
            return;
        }
        String group_avatar = proto.group_avatar();
        Mui32 usrcnt = proto.member_id_list_size();
        Mlog("GroupCreateQ, req_id=%u, group_name=%s, avatar_url=%s, user_cnt=%u ",
            req_user_id, gname.c_str(), group_avatar.c_str(), usrcnt);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            HandleExtData extdata(conn->getID());
            proto.set_user_id(req_user_id);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        else
        {
            Mlog("no DB connection ");
            MBCAF::MsgServer::GroupCreateA proto2;
            proto2.set_user_id(req_user_id);
            proto2.set_result_code(1);
            proto2.set_group_name(gname);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(CreateGroupA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::GroupMemberSQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupMemberSQ proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 change_type = proto.change_type();
        Mui32 req_user_id = conn->getUserID();
        Mui32 gid = proto.group_id();
        Mui32 usrcnt = proto.member_id_list_size();
        Mlog("HandleClientChangeMemberReq, change_type=%u, req_id=%u, group_id=%u, user_cnt=%u ",
            change_type, req_user_id, gid, usrcnt);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            HandleExtData extdata(conn->getID());
            proto.set_user_id(req_user_id);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        else
        {
            Mlog("no DB connection ");
            MBCAF::MsgServer::GroupMemberSA proto2;
            proto2.set_user_id(req_user_id);
            proto2.set_change_type((MBCAF::Proto::GroupModifyType)change_type);
            proto2.set_result_code(1);
            proto2.set_group_id(gid);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(GroupMemberSA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::GroupShieldSQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupShieldSQ proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 shieldstate = proto.shield_status();
        Mui32 gid = proto.group_id();
        Mui32 usrid = conn->getUserID();
        Mlog("GroupShieldSQ, user_id: %u, group_id: %u, shield_status: %u. ",
            usrid, gid, shieldstate);

        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            HandleExtData extdata(conn->getID());
            proto.set_user_id(usrid);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);

        }
        else
        {
            Mlog("no DB connection ");
            MBCAF::MsgServer::GroupShieldSA proto2;
            proto2.set_user_id(usrid);
            proto2.set_result_code(1);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(MSMSG(GroupShieldSA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupShieldSA(MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupShieldSA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 result = proto.result_code();
        Mui32 usrid = proto.user_id();
        Mui32 gid = proto.group_id();
        Mlog("prcGroupShieldSA, result: %u, user_id: %u, group_id: %u. ", result, usrid, gid);

        HandleExtData extdata((Mui8 *)proto.attach_data().c_str(), proto.attach_data().length());
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(usrid, extdata.getHandle());
        if (conn)
        {
            proto.clear_attach_data();
            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::prcGroupShieldA(MdfMessage * msg)
    {
        MBCAF::ServerBase::prcGroupShieldA proto;
        if (!msg->toProto(&proto))
            return;

        Mui32 statelistcnt = proto.shield_status_list_size();

        Mlog("prcGroupShieldA, statelistcnt: %u. ", statelistcnt);

        MBCAF::ServerBase::SwitchTrayMsgQ proto2;
        proto2.set_attach_data((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        for (Mui32 i = 0; i < statelistcnt; i++)
        {
            MBCAF::Proto::ShieldStatus shieldstate = proto.shield_status_list(i);
            if (shieldstate.shield_status() == 0)
            {
                proto2.add_user_id(shieldstate.user_id());
            }
            else
            {
                Mlog("user_id: %u shield group, group id: %u. ", shieldstate.user_id(), shieldstate.group_id());
            }
        }
        MdfMessage remsg;
        remsg.setProto(&proto2);
        remsg.setCommandID(SBMSG(SwitchTrayMsgQ));
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn)
        {
            dbconn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void GroupManager::sendToUser(Mui32 usrid, MdfMessage * msg, ServerConnect * qconn)
    {
        if (msg)
        {
            User * usr = M_Only(UserManager)->getUser(usrid);
            if (usr)
            {
                usr->broadcast(msg, qconn);
            }
        }
    }
    //-----------------------------------------------------------------------
}