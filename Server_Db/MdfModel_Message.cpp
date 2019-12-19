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

#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfServerConnect.h"
#include "MdfModel_Message.h"
#include "MdfModel_Audio.h"
#include "MdfModel_Session.h"
#include "MdfModel_Group.h"
#include "MdfModel_User.h"
#include "MdfEncrypt.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.Proto.pb.h"
#include <time.h>

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    void MessageInfoListA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageInfoListQ proto;
        if (proto->toProto(&msg))
        {
            uint32_t userid = proto.user_id();
            uint32_t sessionid = proto.session_id();
            uint32_t msgid = proto.msg_id_begin();
            uint32_t msgcnt = proto.msg_cnt();
            MBCAF::Proto::SessionType stype = proto.session_type();
            if (MBCAF::Proto::SessionType_IsValid(stype))
            {
                MdfMessage * remsg = new MdfMessage;
                MBCAF::MsgServer::MessageInfoListA protoA;

                list<MBCAF::Proto::MsgInfo> msglist;

                if (stype == MBCAF::Proto::ST_Single)
                {
                    M_Only(Model_Message)->getMessage(userid, sessionid, msgid, msgcnt, msglist);
                }
                else if (stype == MBCAF::Proto::ST_Group)
                {
                    if (M_Only(Model_Group)->isInGroup(userid, sessionid))
                    {
                        M_Only(Model_GroupMessage)->getMessage(userid, sessionid, msgid, msgcnt, msglist);
                    }
                }

                protoA.set_user_id(userid);
                protoA.set_session_id(sessionid);
                protoA.set_msg_id_begin(msgid);
                protoA.set_session_type(stype);
                for (auto it = msglist.begin(); it != msglist.end(); ++it)
                {
                    MBCAF::Proto::MsgInfo * info = protoA.add_msg_list();

                    info->set_msg_id(it->msg_id());
                    info->set_from_session_id(it->from_session_id());
                    info->set_create_time(it->create_time());
                    info->set_msg_type(it->msg_type());
                    info->set_msg_data(it->msg_data());
                }

                Mlog("userId=%u, peerId=%u, msgId=%u, msgCnt=%u, count=%u", userid, sessionid, msgid, msgcnt, protoA.msg_list_size());
                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(MsgListA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("invalid sessionType. userId=%u, peerId=%u, msgId=%u, msgCnt=%u, sessionType=%u",
                    userid, sessionid, msgid, msgcnt, stype);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void SendMessage(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageData proto;
        if (proto->toProto(&msg))
        {
            uint32_t fromid = proto.from_user_id();
            uint32_t toid = proto.to_session_id();
            uint32_t ctime = proto.create_time();
            MBCAF::Proto::MessageType mtype = proto.msg_type();
            uint32_t dsize = proto.msg_data().length();

            uint32_t nNow = (uint32_t)time(NULL);
            if (MBCAF::Proto::MsgType_IsValid(mtype))
            {
                if (dsize != 0)
                {
                    MdfMessage * remsg = new MdfMessage;

                    uint32_t msgid = INVALID_VALUE;
                    uint32_t nSessionId = INVALID_VALUE;
                    uint32_t nPeerSessionId = INVALID_VALUE;

                    Model_Message * M_message = M_Only(Model_Message);
                    Model_GroupMessage * M_group = M_Only(Model_GroupMessage);
                    if (mtype == MBCAF::Proto::MT_GroupText)
                    {
                        Model_Group * M_Group = M_Only(Model_Group);
                        if (M_Group->isValidateGroupId(toid) && M_Group->isInGroup(fromid, toid))
                        {
                            nSessionId = M_Only(Model_Session)->getSessionId(fromid, toid, MBCAF::Proto::ST_Group, false);
                            if (INVALID_VALUE == nSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(fromid, toid, MBCAF::Proto::ST_Group);
                            }
                            if (nSessionId != INVALID_VALUE)
                            {
                                msgid = M_group->getNextMessageId(toid);
                                if (msgid != INVALID_VALUE)
                                {
                                    M_group->sendMessage(fromid, toid, mtype, ctime, msgid, (String&)proto.msg_data());
                                    M_Only(Model_Session)->updateSession(nSessionId, nNow);
                                }
                            }
                        }
                        else
                        {
                            Mlog("invalid gid. fromId=%u, gid=%u", fromid, toid);
                            delete remsg;
                            return;
                        }
                    }
                    else if (mtype == MBCAF::Proto::MT_GroupAudio)
                    {
                        Model_Group * M_Group = M_Only(Model_Group);
                        if (M_Group->isValidateGroupId(toid) && M_Group->isInGroup(fromid, toid))
                        {
                            nSessionId = M_Only(Model_Session)->getSessionId(fromid, toid, MBCAF::Proto::ST_Group, false);
                            if (INVALID_VALUE == nSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(fromid, toid, MBCAF::Proto::ST_Group);
                            }
                            if (nSessionId != INVALID_VALUE)
                            {
                                msgid = M_group->getNextMessageId(toid);
                                if (msgid != INVALID_VALUE)
                                {
                                    M_group->sendAudioMessage(fromid, toid, mtype, ctime, msgid, proto.msg_data().c_str(), dsize);
                                    M_Only(Model_Session)->updateSession(nSessionId, nNow);
                                }
                            }
                        }
                        else
                        {
                            Mlog("invalid gid. fromId=%u, gid=%u", fromid, toid);
                            delete remsg;
                            return;
                        }
                    }
                    else if (mtype == MBCAF::Proto::MT_Text)
                    {
                        if (fromid != toid)
                        {
                            nSessionId = M_Only(Model_Session)->getSessionId(fromid, toid, MBCAF::Proto::ST_Single, false);
                            if (INVALID_VALUE == nSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(fromid, toid, MBCAF::Proto::ST_Single);
                            }
                            nPeerSessionId = M_Only(Model_Session)->getSessionId(toid, fromid, MBCAF::Proto::ST_Single, false);
                            if (INVALID_VALUE == nPeerSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(toid, fromid, MBCAF::Proto::ST_Single);
                            }
                            uint32_t rid = M_Only(Model_Relation)->getRelationId(fromid, toid, true);
                            if (nSessionId != INVALID_VALUE && rid != INVALID_VALUE)
                            {
                                msgid = M_message->getNextMessageId(rid);
                                if (msgid != INVALID_VALUE)
                                {
                                    M_message->sendMessage(rid, fromid, toid, mtype, ctime, msgid, (String&)proto.msg_data());
                                    M_Only(Model_Session)->updateSession(nSessionId, nNow);
                                    M_Only(Model_Session)->updateSession(nPeerSessionId, nNow);
                                }
                                else
                                {
                                    Mlog("msgId is invalid. fromId=%u, toId=%u, relateid=%u, nSessionId=%u, nMsgType=%u", fromid, toid, rid, nSessionId, mtype);
                                }
                            }
                            else
                            {
                                Mlog("sessionId or relateId is invalid. fromId=%u, toId=%u, relateid=%u, nSessionId=%u, nMsgType=%u", fromid, toid, rid, nSessionId, mtype);
                            }
                        }
                        else
                        {
                            Mlog("send msg to self. fromId=%u, toId=%u, msgType=%u", fromid, toid, mtype);
                        }

                    }
                    else if (mtype == MBCAF::Proto::MT_Audio)
                    {

                        if (fromid != toid)
                        {
                            nSessionId = M_Only(Model_Session)->getSessionId(fromid, toid, MBCAF::Proto::ST_Single, false);
                            if (INVALID_VALUE == nSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(fromid, toid, MBCAF::Proto::ST_Single);
                            }
                            nPeerSessionId = M_Only(Model_Session)->getSessionId(toid, fromid, MBCAF::Proto::ST_Single, false);
                            if (INVALID_VALUE == nPeerSessionId)
                            {
                                nSessionId = M_Only(Model_Session)->addSession(toid, fromid, MBCAF::Proto::ST_Single);
                            }
                            uint32_t rid = M_Only(Model_Relation)->getRelationId(fromid, toid, true);
                            if (nSessionId != INVALID_VALUE && rid != INVALID_VALUE)
                            {
                                msgid = M_message->getNextMessageId(rid);
                                if (msgid != INVALID_VALUE)
                                {
                                    M_message->sendAudioMessage(rid, fromid, toid, mtype, ctime, msgid, proto.msg_data().c_str(), dsize);
                                    M_Only(Model_Session)->updateSession(nSessionId, nNow);
                                    M_Only(Model_Session)->updateSession(nPeerSessionId, nNow);
                                }
                                else
                                {
                                    Mlog("msgId is invalid. fromId=%u, toId=%u, relateid=%u, nSessionId=%u, nMsgType=%u", fromid, toid, rid, nSessionId, mtype);
                                }
                            }
                            else
                            {
                                Mlog("sessionId or relateId is invalid. fromId=%u, toId=%u, relateid=%u, nSessionId=%u, nMsgType=%u", fromid, toid, rid, nSessionId, mtype);
                            }
                        }
                        else
                        {
                            Mlog("send msg to self. fromId=%u, toId=%u, msgType=%u", fromid, toid, mtype);
                        }
                    }

                    Mlog("fromId=%u, toId=%u, type=%u, msgId=%u, sessionId=%u", fromid, toid, mtype, msgid, nSessionId);

                    proto.set_msg_id(msgid);
                    remsg->setProto(&proto);
                    remsg->setSeqIdx(msg->getSeqIdx());
                    remsg->setCommandID(MSMSG(Data));
                    M_Only(DataSyncManager)->response(qconn, remsg);
                }
                else
                {
                    Mlog("msgLen error. fromId=%u, toId=%u, msgType=%u", fromid, toid, mtype);
                }
            }
            else
            {
                Mlog("invalid msgType.fromId=%u, toId=%u, msgType=%u", fromid, toid, mtype);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void MessageInfoByIdA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageInfoByIdQ proto;
        if (proto->toProto(&msg))
        {
            uint32_t userid = proto.user_id();
            MBCAF::Proto::SessionType stype = proto.session_type();
            uint32_t sessionid = proto.session_id();
            list<uint32_t> idlist;
            uint32_t nCnt = proto.msg_id_list_size();
            for (uint32_t i = 0; i < nCnt; ++i)
            {
                idlist.push_back(proto.msg_id_list(i));
            }
            if (MBCAF::Proto::SessionType_IsValid(stype))
            {
                MdfMessage* remsg = new MdfMessage;
                MBCAF::MsgServer::MessageInfoByIdA protoA;

                list<MBCAF::Proto::MsgInfo> msglist;

                if (MBCAF::Proto::ST_Single == stype)
                {
                    M_Only(Model_Message)->getMsgByMsgId(userid, sessionid, idlist, msglist);
                }
                else if (MBCAF::Proto::ST_Group == stype)
                {
                    M_Only(Model_GroupMessage)->getMsgByMsgId(userid, sessionid, idlist, msglist);
                }
                protoA.set_user_id(userid);
                protoA.set_session_id(sessionid);
                protoA.set_session_type(stype);
                for (auto it = msglist.begin(); it != msglist.end(); ++it)
                {
                    MBCAF::Proto::MsgInfo * info = protoA.add_msg_list();
                    info->set_msg_id(it->msg_id());
                    info->set_from_session_id(it->from_session_id());
                    info->set_create_time(it->create_time());
                    info->set_msg_type(it->msg_type());
                    info->set_msg_data(it->msg_data());
                }
                Mlog("userId=%u, peerId=%u, sessionType=%u, reqMsgCnt=%u, resMsgCnt=%u", userid, sessionid, stype, proto.msg_id_list_size(), protoA.msg_list_size());
                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(MsgA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("invalid sessionType. fromId=%u, toId=%u, sessionType=%u, msgCnt=%u", userid, sessionid, stype, nCnt);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void MessageRecentA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageRecentQ proto;
        if (msg->toProto(&msg))
        {
            uint32_t userid = proto.user_id();
            MBCAF::Proto::SessionType stype = proto.session_type();
            uint32_t sessionid = proto.session_id();
            if (MBCAF::Proto::SessionType_IsValid(stype))
            {
                MdfMessage * remsg = new MdfMessage;
                MBCAF::MsgServer::MessageRecentA protoA;
                protoA.set_user_id(userid);
                protoA.set_session_type(stype);
                protoA.set_session_id(sessionid);
                uint32_t msgid = INVALID_VALUE;
                if (MBCAF::Proto::ST_Single == stype)
                {
                    String strMsg;
                    MBCAF::Proto::MessageType mtype;
                    M_Only(Model_Message)->getLastMsg(userid, sessionid, msgid, strMsg, mtype, 1);
                }
                else
                {
                    String strMsg;
                    MBCAF::Proto::MessageType mtype;
                    uint32_t fromid = INVALID_VALUE;
                    M_Only(Model_GroupMessage)->getLastMsg(sessionid, msgid, strMsg, mtype, fromid);
                }
                protoA.set_latest_msg_id(msgid);
                Mlog("userId=%u, peerId=%u, sessionType=%u, msgId=%u", userid, sessionid, stype, msgid);
                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(RecentMsgA0);
                M_Only(DataSyncManager)->response(qconn, remsg);

            }
            else
            {
                Mlog("invalid sessionType. userId=%u, peerId=%u, sessionType=%u", userid, sessionid, stype);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void MessageUnreadCntA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageUnreadCntQ proto;
        MBCAF::MsgServer::MessageUnreadCntA protoA;
        if (proto->toProto(&msg))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t userid = proto.user_id();

            list<MBCAF::Proto::UnreadInfo> infolist;
            uint32_t nTotalCnt = 0;

            M_Only(Model_Message)->getUnreadMessage(userid, nTotalCnt, infolist);
            M_Only(Model_GroupMessage)->getUnreadMessage(userid, nTotalCnt, infolist);
            protoA.set_user_id(userid);
            protoA.set_total_cnt(nTotalCnt);
            for (auto it = infolist.begin(); it != infolist.end(); ++it)
            {
                MBCAF::Proto::UnreadInfo * pInfo = protoA.add_unreadinfo_list();
                pInfo->set_session_id(it->session_id());
                pInfo->set_session_type(it->session_type());
                pInfo->set_unread_cnt(it->unread_cnt());
                pInfo->set_latest_msg_id(it->latest_msg_id());
                pInfo->set_latest_msg_data(it->latest_msg_data());
                pInfo->set_latest_msg_type(it->latest_msg_type());
                pInfo->set_latest_msg_from_user_id(it->latest_msg_from_user_id());
            }

            Mlog("userId=%d, unreadCnt=%u, totalCount=%u", userid, protoA.unreadinfo_list_size(), nTotalCnt);
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(UnReadCountA));
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void MessageReadA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::MessageReadA proto;
        if (proto->toProto(&msg))
        {
            uint32_t userid = proto.user_id();
            uint32_t fromid = proto.session_id();
            MBCAF::Proto::SessionType stype = proto.session_type();
            M_Only(Model_User)->clearUserCounter(userid, fromid, stype);
            Mlog("userId=%u, peerId=%u, type=%u", fromid, userid, stype);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void TrayMsgA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::ServerBase::TrayMsgQ proto;
        MBCAF::ServerBase::TrayMsgA protoA;
        if (proto->toProto(&msg))
        {
            uint32_t userid = proto.user_id();
            String strToken = proto.device_token();
            MdfMessage * remsg = new MdfMessage;
            MembaseManager * memdbMag = M_Only(MembaseManager);
            MembaseConnect * memdbConn = memdbMag->getTempConnect("token");
            if (memdbConn)
            {
                MBCAF::Proto::ClientType ctype = proto.client_type();
                String strValue;
                if (MBCAF::Proto::CT_IOS == ctype)
                {
                    strValue = "ios:" + strToken;
                }
                else if (MBCAF::Proto::CT_Android == ctype)
                {
                    strValue = "android:" + strToken;
                }
                else
                {
                    strValue = strToken;
                }

                String ovalue = memdbConn->get("device_" + itostr(userid));
                if (!ovalue.empty())
                {
                    size_t nPos = ovalue.find(":");
                    if (nPos != String::npos)
                    {
                        String strOldToken = ovalue.substr(nPos + 1);
                        String strReply = memdbConn->get("device_" + strOldToken);
                        if (!strReply.empty())
                        {
                            String strNewValue("");
                            memdbConn->set("device_" + strOldToken, strNewValue);
                        }
                    }
                }
                memdbConn->set("device_" + itostr(userid), strValue);
                String strNewValue = itostr(userid);
                memdbConn->set("device_" + strToken, strNewValue);

                Mlog("setDeviceToken. userId=%u, deviceToken=%s", userid, strToken.c_str());
                memdbMag->freeTempConnect(memdbConn);
            }
            else
            {
                Mlog("no cache connection for token");
            }

            Mlog("setDeviceToken. userId=%u, deviceToken=%s", userid, strToken.c_str());
            protoA.set_attach_data(proto.attach_data());
            protoA.set_user_id(userid);
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(SBMSG(TrayMsgA));
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void SwitchTrayMsgA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::ServerBase::SwitchTrayMsgQ proto;
        MBCAF::ServerBase::SwitchTrayMsgA protoA;
        if (proto->toProto(&msg))
        {
            MembaseManager * memdbMag = M_Only(MembaseManager);
            MembaseConnect * memdbConn = memdbMag->getTempConnect("token");
            MdfMessage * remsg = new MdfMessage;
            uint32_t nCnt = proto.user_id_size();

            bool is_check_shield_status = false;
            time_t now = time(NULL);
            struct tm * _tm = localtime(&now);
            if (_tm->tm_hour >= 22 || _tm->tm_hour <= 7)
            {
                is_check_shield_status = true;
            }
            if (memdbConn)
            {
                vector<String> vecTokens;
                for (uint32_t i = 0; i < nCnt; ++i)
                {
                    String strKey = "device_" + itostr(proto.user_id(i));
                    vecTokens.push_back(strKey);
                }
                map<String, String> mapTokens;
                bool bRet = memdbConn->MGET(vecTokens, mapTokens);
                memdbMag->freeTempConnect(memdbConn);

                if (bRet)
                {
                    for (auto it = mapTokens.begin(); it != mapTokens.end(); ++it)
                    {
                        String strKey = it->first;
                        size_t nPos = strKey.find("device_");
                        if (nPos != String::npos)
                        {
                            String strUserId = strKey.substr(nPos + strlen("device_"));
                            uint32_t userid = strtoi(strUserId);
                            String strValue = it->second;
                            nPos = strValue.find(":");
                            if (nPos != String::npos)
                            {
                                String strType = strValue.substr(0, nPos);
                                String strToken = strValue.substr(nPos + 1);
                                MBCAF::Proto::ClientType ctype = MBCAF::Proto::ClientType(0);
                                if (strType == "ios")
                                {
                                    uint32_t shield_status = 0;
                                    if (is_check_shield_status)
                                    {
                                        M_Only(Model_User)->getPushShield(userid, &shield_status);
                                    }

                                    if (shield_status == 1)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        ctype = MBCAF::Proto::CT_IOS;
                                    }
                                }
                                else if (strType == "android")
                                {
                                    ctype = MBCAF::Proto::CT_Android;
                                }
                                if (MBCAF::Proto::ClientType_IsValid(ctype))
                                {
                                    MBCAF::Proto::UserTokenInfo * pToken = protoA.add_user_token_info();
                                    pToken->set_user_id(userid);
                                    pToken->set_token(strToken);
                                    pToken->set_user_type(ctype);
                                    uint32_t nTotalCnt = 0;
                                    M_Only(Model_Message)->getUserUnread(userid, nTotalCnt);
                                    M_Only(Model_GroupMessage)->getUserUnread(userid, nTotalCnt);
                                    pToken->set_push_count(nTotalCnt);
                                    pToken->set_push_type(1);
                                }
                                else
                                {
                                    Mlog("invalid clientType.clientType=%u", ctype);
                                }
                            }
                            else
                            {
                                Mlog("invalid value. value=%s", strValue.c_str());
                            }
                        }
                        else
                        {
                            Mlog("invalid key.key=%s", strKey.c_str());
                        }
                    }
                }
                else
                {
                    Mlog("MGET failed!");
                }
            }
            else
            {
                Mlog("no cache connection for token");
            }

            Mlog("req devices token.reqCnt=%u, resCnt=%u", nCnt, protoA.user_token_info_size());

            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(SBMSG(SwitchTrayMsgA0);
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Model_Relation
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_Relation);
    //-----------------------------------------------------------------------
    Model_Relation::Model_Relation()
    {
    }
    //-----------------------------------------------------------------------
    Model_Relation::~Model_Relation()
    {
    }
    //-----------------------------------------------------------------------
    uint32_t Model_Relation::getRelationId(uint32_t aid, uint32_t bid, bool add)
    {
        uint32_t nRelationId = INVALID_VALUE;
        if (aid == 0 || bid == 0)
        {
            Mlog("invalied user id:%u->%u", aid, bid);
            return nRelationId;
        }
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            uint32_t bignum = aid > bid ? aid : bid;
            uint32_t smallnum = aid > bid ? bid : aid;
            String strSql = "select id from MACAF_SessionRelation where uid1=" + itostr(smallnum) + " and uid2=" + itostr(bignum) + " and state = 0";

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    resSet->getValue("id", nRelationId);
                }
                delete resSet;
            }
            else
            {
                Mlog("there is no result for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
            if (nRelationId == INVALID_VALUE && add)
            {
                dbConn = dbMag->getTempConnect("gsgsmaster");
                if (dbConn)
                {
                    uint32_t nTimeNow = (uint32_t)time(NULL);
                    String strSql = "select id from MACAF_SessionRelation where uid1=" + itostr(smallnum) + " and uid2=" + itostr(bignum);
                    DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
                    if (resSet && resSet->nextRow())
                    {
                        resSet->getValue("id", nRelationId);
                        strSql = "update MACAF_SessionRelation set state=0, updated=" + itostr(nTimeNow) + " where id=" + itostr(nRelationId);
                        bool bRet = dbConn->exec(strSql.c_str());
                        if (!bRet)
                        {
                            nRelationId = INVALID_VALUE;
                        }
                        Mlog("has relation ship set state");
                        delete resSet;
                    }
                    else
                    {
                        strSql = "insert into MACAF_SessionRelation (`uid1`,`uid2`,`state`,`created`,`updated`) values(?,?,?,?,?)";
                        PrepareExec * stmt = new PrepareExec();
                        if (stmt->prepare(dbConn->getConnect(), strSql))
                        {
                            uint32_t mState = 0;
                            uint32_t index = 0;
                            stmt->setParam(index++, smallnum);
                            stmt->setParam(index++, bignum);
                            stmt->setParam(index++, mState);
                            stmt->setParam(index++, nTimeNow);
                            stmt->setParam(index++, nTimeNow);
                            bool bRet = stmt->exec();
                            if (bRet)
                            {
                                nRelationId = dbConn->getInsertId();
                            }
                            else
                            {
                                Mlog("insert message failed. %s", strSql.c_str());
                            }
                        }
                        if (nRelationId != INVALID_VALUE)
                        {
                            if (!M_Only(Model_Message)->resetMessageID(nRelationId))
                            {
                                Mlog("reset msgId failed. uid1=%u, uid2=%u.", smallnum, bignum);
                            }
                        }
                        delete stmt;
                    }
                    dbMag->freeTempConnect(dbConn);
                }
                else
                {
                    Mlog("no db connection for gsgsmaster");
                }
            }
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
        return nRelationId;
    }
    //-----------------------------------------------------------------------
    bool Model_Relation::updateRelation(uint32_t nRelationId, uint32_t nUpdateTime)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "update MACAF_SessionRelation set `updated`=" + itostr(nUpdateTime) + " where id=" + itostr(nRelationId);
            bRet = dbConn->exec(strSql.c_str());
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Relation::removeRelation(uint32_t nRelationId)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            uint32_t nNow = (uint32_t)time(NULL);
            String strSql = "update MACAF_SessionRelation set state = 1, updated=" + itostr(nNow) + " where id=" + itostr(nRelationId);
            bRet = dbConn->exec(strSql.c_str());
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Model_Message
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_Message);
    //-----------------------------------------------------------------------
    Model_Message::Model_Message()
    {
    }
    //-----------------------------------------------------------------------
    Model_Message::~Model_Message()
    {
    }
    //-----------------------------------------------------------------------
    void Model_Message::setupEncryptStr(const String & str)
    {
        AES aesobj = AES(str.c_str());
        String strAudio = "[gsgsaudio]";
        char * edata;
        uint32_t edsize;
        if (aesobj.Encrypt(strAudio.c_str(), strAudio.length(), &edata, edsize) == 0)
        {
            AudioEncryptStr.clear();
            AudioEncryptStr.append(edata, edsize);
            aesobj.Free(edata);
        }
    }
    //-----------------------------------------------------------------------
    void Model_Message::getMessage(uint32_t userid, uint32_t sessionid, uint32_t msgid,
        uint32_t msgcnt, list<MBCAF::Proto::MsgInfo> & msglist)
    {
        uint32_t rid = M_Only(Model_Relation)->getRelationId(userid, sessionid, false);
        if (rid != INVALID_VALUE)
        {
            DatabaseManager* dbMag = M_Only(DatabaseManager);
            DatabaseConnect* dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                String strTableName = "MACAF_Message" + itostr(rid % 8);
                String strSql;
                if (msgid == 0)
                {
                    strSql = "select * from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + itostr(rid) + " and state = 0 order by created desc, id desc limit " + itostr(msgcnt);
                }
                else
                {
                    strSql = "select * from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + itostr(rid) + " and state = 0 and msgId <=" + itostr(msgid) + " order by created desc, id desc limit " + itostr(msgcnt);
                }
                DatabaseResult* resSet = dbConn->execQuery(strSql.c_str());
                if (resSet)
                {
                    while (resSet->nextRow())
                    {
                        MBCAF::Proto::MsgInfo cMsg;
                        Mi32 temp2;
                        resSet->getValue("msgId", temp2);
                        cMsg.set_msg_id(temp2);
                        resSet->getValue("fromId", temp2);
                        cMsg.set_from_session_id(temp2);
                        resSet->getValue("created", temp2);
                        cMsg.set_create_time(temp2);
                        resSet->getValue("type", temp2);
                        MBCAF::Proto::MessageType mtype = MBCAF::Proto::MessageType(temp2);
                        if (MBCAF::Proto::MsgType_IsValid(mtype))
                        {
                            cMsg.set_msg_type(mtype);
                            cMsg.set_msg_data(resSet->getValue("content"));
                            msglist.push_back(cMsg);
                        }
                        else
                        {
                            Mlog("invalid msgType. userId=%u, peerId=%u, msgId=%u, msgCnt=%u, msgType=%u", userid, sessionid, msgid, msgcnt, mtype);
                        }
                    }
                    delete resSet;
                }
                else
                {
                    Mlog("no result set: %s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);
                if (!msglist.empty())
                {
                    M_Only(Model_Audio)->readAudios(msglist);
                }
            }
            else
            {
                Mlog("no db connection for gsgsslave");
            }
        }
        else
        {
            Mlog("no relation between %lu and %lu", userid, sessionid);
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Message::sendMessage(uint32_t rid, uint32_t fromid, uint32_t toid, 
        MBCAF::Proto::MessageType mtype, uint32_t ctime, uint32_t msgid, String & data)
    {
        bool bRet = false;
        if (fromid == 0 || toid == 0)
        {
            Mlog("invalied userId.%u->%u", fromid, toid);
            return bRet;
        }

        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strTableName = "MACAF_Message" + itostr(rid % 8);
            String strSql = "insert into " + strTableName + " (`relateId`, `fromId`, `toId`, `msgId`, `content`, `state`, `type`, `created`, `updated`) values(?, ?, ?, ?, ?, ?, ?, ?, ?)";

            PrepareExec * pStmt = new PrepareExec();
            if (pStmt->prepare(dbConn->getConnect(), strSql))
            {
                uint32_t mState = 0;
                uint32_t index = 0;
                pStmt->setParam(index++, rid);
                pStmt->setParam(index++, fromid);
                pStmt->setParam(index++, toid);
                pStmt->setParam(index++, msgid);
                pStmt->setParam(index++, data);
                pStmt->setParam(index++, mState);
                pStmt->setParam(index++, (uint32_t)mtype);
                pStmt->setParam(index++, ctime);
                pStmt->setParam(index++, ctime);
                bRet = pStmt->exec();
            }
            delete pStmt;
            dbMag->freeTempConnect(dbConn);
            if (bRet)
            {
                uint32_t nNow = (uint32_t)time(NULL);
                MembaseManager * memdbMag = M_Only(MembaseManager);
                MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
                if (memdbConn)
                {
                    memdbConn->HINCRBY("unread_" + itostr(toid), itostr(fromid), 1);
                    memdbMag->freeTempConnect(memdbConn);
                }
                else
                {
                    Mlog("no cache connection to increase unread count: %d->%d", fromid, toid);
                }
            }
            else
            {
                Mlog("insert message failed: %s", strSql.c_str());
            }
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Message::sendAudioMessage(uint32_t rid, uint32_t fromid, uint32_t toid, 
        MBCAF::Proto::MessageType mtype, uint32_t ctime, uint32_t msgid, const char * data, uint32_t dsize)
    {
        if (dsize <= 4)
        {
            return false;
        }

        Model_Audio * M_Audio = M_Only(Model_Audio);
        int nAudioId = M_Audio->saveAudioInfo(fromid, toid, ctime, data, dsize);

        bool bRet = true;
        if (nAudioId != -1)
        {
            String strMsg = itostr(nAudioId);
            bRet = sendMessage(rid, fromid, toid, mtype, ctime, msgid, strMsg);
        }
        else
        {
            bRet = false;
        }

        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_Message::getUnreadMessage(uint32_t userid, uint32_t & nTotalCnt, list<MBCAF::Proto::UnreadInfo>& infolist)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            map<String, String> mapUnread;
            String strKey = "unread_" + itostr(userid);
            bool bRet = memdbConn->HGETALL(strKey, mapUnread);
            memdbMag->freeTempConnect(memdbConn);
            if (bRet)
            {
                MBCAF::Proto::UnreadInfo cUnreadInfo;
                for (auto it = mapUnread.begin(); it != mapUnread.end(); it++)
                {
                    cUnreadInfo.set_session_id(atoi(it->first.c_str()));
                    cUnreadInfo.set_unread_cnt(atoi(it->second.c_str()));
                    cUnreadInfo.set_session_type(MBCAF::Proto::ST_Single);
                    uint32_t msgid = 0;
                    String strMsgData;
                    MBCAF::Proto::MessageType mtype;
                    getLastMsg(cUnreadInfo.session_id(), userid, msgid, strMsgData, mtype);
                    if (MBCAF::Proto::MsgType_IsValid(mtype))
                    {
                        cUnreadInfo.set_latest_msg_id(msgid);
                        cUnreadInfo.set_latest_msg_data(strMsgData);
                        cUnreadInfo.set_latest_msg_type(mtype);
                        cUnreadInfo.set_latest_msg_from_user_id(cUnreadInfo.session_id());
                        infolist.push_back(cUnreadInfo);
                        nTotalCnt += cUnreadInfo.unread_cnt();
                    }
                    else
                    {
                        Mlog("invalid msgType. userId=%u, peerId=%u, msgType=%u", userid, cUnreadInfo.session_id(), mtype);
                    }
                }
            }
            else
            {
                Mlog("hgetall %s failed!", strKey.c_str());
            }
        }
        else
        {
            Mlog("no cache connection for unread");
        }
    }
    //-----------------------------------------------------------------------
    uint32_t Model_Message::getNextMessageId(uint32_t rid)
    {
        uint32_t msgid = 0;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            String strKey = "msg_id_" + itostr(rid);
            msgid = memdbConn->INCRBY(strKey, 1);
            memdbMag->freeTempConnect(memdbConn);
        }
        return msgid;
    }
    //-----------------------------------------------------------------------
    void Model_Message::getLastMsg(uint32_t fromid, uint32_t toid, uint32_t & msgid, 
        String & strMsgData, MBCAF::Proto::MessageType & mtype, uint32_t mState)
    {
        uint32_t rid = M_Only(Model_Relation)->getRelationId(fromid, toid, false);
        if (rid != INVALID_VALUE)
        {
            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                String strTableName = "MACAF_Message" + itostr(rid % 8);
                String strSql = "select msgId, type, content from " + strTableName + " force index (idx_relateId_status_created) where relateId= " + itostr(rid) + " and state = 0 order by created desc, id desc limit 1";
                DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
                if (resSet)
                {
                    while (resSet->nextRow())
                    {
                        resSet->getValue("msgId", msgid);
                        Mi32 temp2;
                        resSet->getValue("type", temp2);
                        mtype = MBCAF::Proto::MessageType(temp2);
                        if (mtype == MBCAF::Proto::MT_Audio)
                        {
                            strMsgData = AudioEncryptStr;
                        }
                        else
                        {
                            strMsgData = resSet->getValue("content");
                        }
                    }
                    delete resSet;
                }
                else
                {
                    Mlog("no result set: %s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection_slave");
            }
        }
        else
        {
            Mlog("no relation between %lu and %lu", fromid, toid);
        }
    }
    //-----------------------------------------------------------------------
    void Model_Message::getUserUnread(uint32_t userid, uint32_t & nTotalCnt)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            map<String, String> mapUnread;
            String strKey = "unread_" + itostr(userid);
            bool bRet = memdbConn->HGETALL(strKey, mapUnread);
            memdbMag->freeTempConnect(memdbConn);

            if (bRet)
            {
                for (auto it = mapUnread.begin(); it != mapUnread.end(); it++)
                {
                    nTotalCnt += atoi(it->second.c_str());
                }
            }
            else
            {
                Mlog("hgetall %s failed!", strKey.c_str());
            }
        }
        else
        {
            Mlog("no cache connection for unread");
        }
    }
    //-----------------------------------------------------------------------
    void Model_Message::getMsgByMsgId(uint32_t userid, uint32_t sessionid, const list<uint32_t> & idlist, list<MBCAF::Proto::MsgInfo> &msglist)
    {
        if (idlist.empty())
        {
            return;
        }
        uint32_t rid = M_Only(Model_Relation)->getRelationId(userid, sessionid, false);

        if (rid == INVALID_VALUE)
        {
            Mlog("invalid relation id between %u and %u", userid, sessionid);
            return;
        }

        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strTableName = "MACAF_Message" + itostr(rid % 8);
            String strClause;
            bool bFirst = true;
            for (auto it = idlist.begin(); it != idlist.end(); ++it)
            {
                if (bFirst)
                {
                    bFirst = false;
                    strClause = itostr(*it);
                }
                else
                {
                    strClause += ("," + itostr(*it));
                }
            }

            String strSql = "select * from " + strTableName + " where relateId=" + itostr(rid) + "  and state=0 and msgId in (" + strClause + ") order by created desc, id desc limit 100";
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    MBCAF::Proto::MsgInfo proto;
                    Mi32 temp2;
                    resSet->getValue("msgId", temp2);
                    proto.set_msg_id(temp2);
                    resSet->getValue("fromId", temp2);
                    proto.set_from_session_id(temp2);
                    resSet->getValue("created", temp2);
                    proto.set_create_time(temp2);
                    resSet->getValue("type", temp2);
                    MBCAF::Proto::MessageType mtype = MBCAF::Proto::MessageType(temp2);
                    if (MBCAF::Proto::MsgType_IsValid(mtype))
                    {
                        proto.set_msg_type(mtype);
                        proto.set_msg_data(resSet->getValue("content"));
                        msglist.push_back(proto);
                    }
                    else
                    {
                        Mlog("invalid msgType. userId=%u, peerId=%u, msgType=%u, msgId=%u", userid, sessionid, mtype, proto.msg_id());
                    }
                }
                delete resSet;
            }
            else
            {
                Mlog("no result set for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
            if (!msglist.empty())
            {
                M_Only(Model_Audio)->readAudios(msglist);
            }
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Message::resetMessageID(uint32_t rid)
    {
        bool re = false;
        uint32_t msgid = 0;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            String strKey = "msg_id_" + itostr(rid);
            String strValue = "0";
            String strReply = memdbConn->set(strKey, strValue);
            if (strReply == strValue)
            {
                re = true;
            }
            memdbMag->freeTempConnect(memdbConn);
        }
        return re;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Model_GroupMessage
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_GroupMessage);
    //-----------------------------------------------------------------------
    Model_GroupMessage::Model_GroupMessage()
    {
    }
    //-----------------------------------------------------------------------
    Model_GroupMessage::~Model_GroupMessage()
    {
    }
    //-----------------------------------------------------------------------
    bool Model_GroupMessage::sendMessage(uint32_t fromid, uint32_t gid, MBCAF::Proto::MessageType mtype,
        uint32_t ctime, uint32_t msgid, const String& data)
    {
        bool bRet = false;
        if (M_Only(Model_Group)->isInGroup(fromid, gid))
        {
            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
            if (dbConn)
            {
                String strTableName = "MACAF_GroupMessage" + itostr(gid % 8);
                String strSql = "insert into " + strTableName + " (`gid`, `userId`, `msgId`, `content`, `type`, `state`, `updated`, `created`) "\
                    "values(?, ?, ?, ?, ?, ?, ?, ?)";

                PrepareExec* pStmt = new PrepareExec();
                if (pStmt->prepare(dbConn->getConnect(), strSql))
                {
                    uint32_t mState = 0;
                    uint32_t type = mtype;
                    uint32_t index = 0;
                    pStmt->setParam(index++, gid);
                    pStmt->setParam(index++, fromid);
                    pStmt->setParam(index++, msgid);
                    pStmt->setParam(index++, data);
                    pStmt->setParam(index++, type);
                    pStmt->setParam(index++, mState);
                    pStmt->setParam(index++, ctime);
                    pStmt->setParam(index++, ctime);

                    bool bRet = pStmt->exec();
                    if (bRet)
                    {
                        M_Only(Model_Group)->updateGroupChat(gid);
                        MembaseManager * memdbMag = M_Only(MembaseManager);
                        MembaseConnect * memConn = memdbMag->getTempConnect("unread");
                        if (memConn)
                        {
                            String strGroupKey = itostr(gid) + ChatGroupMsg;
                            memConn->HINCRBY(strGroupKey, ChatMsgCount, 1);
                            map<String, String> mapGroupCount;
                            bool bRet = memConn->HGETALL(strGroupKey, mapGroupCount);
                            if (bRet)
                            {
                                String strUserKey = itostr(fromid) + "_" + itostr(gid) + ChatUserGroupMsg;
                                String strReply = memConn->HMSET(strUserKey, mapGroupCount);
                                if (strReply.empty())
                                {
                                    Mlog("HMSET %s failed !", strUserKey.c_str());
                                }
                            }
                            else
                            {
                                Mlog("HGETALL %s failed!", strGroupKey.c_str());
                            }
                            memdbMag->freeTempConnect(memConn);
                        }
                        else
                        {
                            Mlog("no cache connection for unread");
                        }
                        memConn = memdbMag->getTempConnect("unread");
                        if (memConn)
                        {
                            String strGroupKey = itostr(gid) + ChatGroupMsg;
                            map<String, String> mapGroupCount;
                            bool bRet = memConn->HGETALL(strGroupKey, mapGroupCount);
                            memdbMag->freeTempConnect(memConn);
                            if (bRet)
                            {
                                String strUserKey = itostr(fromid) + "_" + itostr(gid) + ChatUserGroupMsg;
                                String strReply = memConn->HMSET(strUserKey, mapGroupCount);
                                if (strReply.empty())
                                {
                                    Mlog("HMSET %s failed !", strUserKey.c_str());
                                }
                            }
                            else
                            {
                                Mlog("HGETALL %s failed !", strGroupKey.c_str());
                            }
                        }
                        else
                        {
                            Mlog("no cache connection for unread");
                        }
                    }
                    else
                    {
                        Mlog("insert message failed: %s", strSql.c_str());
                    }
                }
                delete pStmt;
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsmaster");
            }
        }
        else
        {
            Mlog("not in the group.fromId=%u, gid=%u", fromid, gid);
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_GroupMessage::sendAudioMessage(uint32_t fromid, uint32_t gid, MBCAF::Proto::MessageType mtype,
        uint32_t ctime, uint32_t msgid, const char* data, uint32_t dsize)
    {
        if (dsize <= 4)
        {
            return false;
        }

        if (!M_Only(Model_Group)->isInGroup(fromid, gid))
        {
            Mlog("not in the group.fromId=%u, gid=%u", fromid, gid);
            return false;
        }

        Model_Audio * M_Audio = M_Only(Model_Audio);
        int nAudioId = M_Audio->saveAudioInfo(fromid, gid, ctime, data, dsize);

        bool bRet = true;
        if (nAudioId != -1)
        {
            String strMsg = itostr(nAudioId);
            bRet = sendMessage(fromid, gid, mtype, ctime, msgid, strMsg);
        }
        else
        {
            bRet = false;
        }

        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_GroupMessage::getMessage(uint32_t userid, uint32_t gid, uint32_t msgid, uint32_t msgcnt, list<MBCAF::Proto::MsgInfo>& msglist)
    {
        String strTableName = "MACAF_GroupMessage" + itostr(gid % 8);
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            uint32_t nUpdated = M_Only(Model_Group)->getUserJoinTime(gid, userid);
            String strSql;
            if (msgid == 0)
            {
                strSql = "select * from " + strTableName + " where gid = " + itostr(gid) + " and state = 0 and created>=" + itostr(nUpdated) + " order by created desc, id desc limit " + itostr(msgcnt);
            }
            else
            {
                strSql = "select * from " + strTableName + " where gid = " + itostr(gid) + " and msgId<=" + itostr(msgid) + " and state = 0 and created>=" + itostr(nUpdated) + " order by created desc, id desc limit " + itostr(msgcnt);
            }

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                map<uint32_t, MBCAF::Proto::MsgInfo> mapAudioMsg;
                while (resSet->nextRow())
                {
                    MBCAF::Proto::MsgInfo proto;
                    Mi32 temp1;
                    resSet->getValue("msgId", temp1);
                    proto.set_msg_id(temp1);
                    resSet->getValue("userId", temp1);
                    proto.set_from_session_id(temp1);
                    resSet->getValue("created", temp1);
                    proto.set_create_time(temp1);
                    resSet->getValue("type", temp1);
                    MBCAF::Proto::MessageType mtype = MBCAF::Proto::MessageType(temp1);
                    if(MBCAF::Proto::MsgType_IsValid(mtype))
                    {
                        String temp;
                        resSet->getValue("content", temp);
                        proto.set_msg_type(mtype);
                        proto.set_msg_data(temp.c_str());
                        msglist.push_back(proto);
                    }
                    else
                    {
                        Mlog("invalid msgType. userId=%u, gid=%u, msgType=%u", userid, gid, mtype);
                    }
                }
                delete resSet;
            }
            else
            {
                Mlog("no result set for sql: %s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
            if (!msglist.empty())
            {
                M_Only(Model_Audio)->readAudios(msglist);
            }
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    void Model_GroupMessage::getUnreadMessage(uint32_t userid, uint32_t & nTotalCnt, list<MBCAF::Proto::UnreadInfo> & infolist)
    {
        list<uint32_t> lsGroupId;
        M_Only(Model_Group)->getUserGroup(userid, lsGroupId, 0);
        uint32_t nCount = 0;

        MembaseManager* memdbMag = M_Only(MembaseManager);
        MembaseConnect* memConn = memdbMag->getTempConnect("unread");
        if (memConn)
        {
            for (auto it = lsGroupId.begin(); it != lsGroupId.end(); ++it)
            {
                uint32_t gid = *it;
                String strGroupKey = itostr(gid) + ChatGroupMsg;
                String strGroupCnt = memConn->HGET(strGroupKey, ChatMsgCount);
                if (strGroupCnt.empty())
                {
                    continue;
                }
                uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));

                String strUserKey = itostr(userid) + "_" + itostr(gid) + ChatUserGroupMsg;
                String strUserCnt = memConn->HGET(strUserKey, ChatMsgCount);

                uint32_t nUserCnt = (strUserCnt.empty() ? 0 : ((uint32_t)atoi(strUserCnt.c_str())));
                if (nGroupCnt >= nUserCnt)
                {
                    nCount = nGroupCnt - nUserCnt;
                }
                if (nCount > 0)
                {
                    MBCAF::Proto::UnreadInfo cUnreadInfo;
                    cUnreadInfo.set_session_id(gid);
                    cUnreadInfo.set_session_type(MBCAF::Proto::ST_Group);
                    cUnreadInfo.set_unread_cnt(nCount);
                    nTotalCnt += nCount;
                    String strMsgData;
                    uint32_t msgid;
                    MBCAF::Proto::MessageType type;
                    uint32_t fromid;
                    getLastMsg(gid, msgid, strMsgData, type, fromid);
                    if (MBCAF::Proto::MsgType_IsValid(type))
                    {
                        cUnreadInfo.set_latest_msg_id(msgid);
                        cUnreadInfo.set_latest_msg_data(strMsgData);
                        cUnreadInfo.set_latest_msg_type(type);
                        cUnreadInfo.set_latest_msg_from_user_id(fromid);
                        infolist.push_back(cUnreadInfo);
                    }
                    else
                    {
                        Mlog("invalid msgType. userId=%u, gid=%u, msgType=%u, msgId=%u", userid, gid, type, msgid);
                    }
                }
            }
            memdbMag->freeTempConnect(memConn);
        }
        else
        {
            Mlog("no cache connection for unread");
        }
    }
    //-----------------------------------------------------------------------
    uint32_t Model_GroupMessage::getNextMessageId(uint32_t gid)
    {
        uint32_t msgid = 0;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memConn = memdbMag->getTempConnect("unread");
        if (memConn)
        {
            String strKey = "group_msg_id_" + itostr(gid);
            msgid = memConn->INCRBY(strKey, 1);
            memdbMag->freeTempConnect(memConn);
        }
        else
        {
            Mlog("no cache connection for unread");
        }
        return msgid;
    }
    //-----------------------------------------------------------------------
    void Model_GroupMessage::getLastMsg(uint32_t gid, uint32_t & msgid, String &strMsgData, 
        MBCAF::Proto::MessageType &mtype, uint32_t& fromid)
    {
        String strTableName = "MACAF_GroupMessage" + itostr(gid % 8);

        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select msgId, type,userId, content from " + strTableName + " where gid = " + itostr(gid) + " and state = 0 order by created desc, id desc limit 1";

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    Mi32 temp1;
                    resSet->getValue("msgId", msgid);
                    resSet->getValue("type", temp1);
                    resSet->getValue("userId", fromid);

                    mtype = MBCAF::Proto::MessageType(temp1);
                    if (mtype == MBCAF::Proto::MT_GroupAudio)
                    {
                        strMsgData = M_Only(Model_Message)->AudioEncryptStr;
                    }
                    else
                    {
                        resSet->getValue("content", strMsgData);
                    }
                }
                delete resSet;
            }
            else
            {
                Mlog("no result set for sql: %s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    void Model_GroupMessage::getUserUnread(uint32_t userid, uint32_t & nTotalCnt)
    {
        list<uint32_t> lsGroupId;
        M_Only(Model_Group)->getUserGroup(userid, lsGroupId, 0);
        uint32_t nCount = 0;

        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memConn = memdbMag->getTempConnect("unread");
        if (memConn)
        {
            for (auto it = lsGroupId.begin(); it != lsGroupId.end(); ++it)
            {
                uint32_t gid = *it;
                String strGroupKey = itostr(gid) + ChatGroupMsg;
                String strGroupCnt = memConn->HGET(strGroupKey, ChatMsgCount);
                if (strGroupCnt.empty())
                {
                    continue;
                }
                uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));

                String strUserKey = itostr(userid) + "_" + itostr(gid) + ChatUserGroupMsg;
                String strUserCnt = memConn->HGET(strUserKey, ChatMsgCount);

                uint32_t nUserCnt = (strUserCnt.empty() ? 0 : ((uint32_t)atoi(strUserCnt.c_str())));
                if (nGroupCnt >= nUserCnt)
                {
                    nCount = nGroupCnt - nUserCnt;
                }
                if (nCount > 0)
                {
                    nTotalCnt += nCount;
                }
            }
            memdbMag->freeTempConnect(memConn);
        }
        else
        {
            Mlog("no cache connection for unread");
        }
    }
    //-----------------------------------------------------------------------
    void Model_GroupMessage::getMsgByMsgId(uint32_t userid, uint32_t gid, const list<uint32_t> &idlist, list<MBCAF::Proto::MsgInfo> &msglist)
    {
        if(!idlist.empty())
        {
            if(M_Only(Model_Group)->isInGroup(userid, gid))
            {
                DatabaseManager * dbMag = M_Only(DatabaseManager);
                DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
                if (dbConn)
                {
                    String strTableName = "MACAF_GroupMessage" + itostr(gid % 8);
                    uint32_t nUpdated = M_Only(Model_Group)->getUserJoinTime(gid, userid);
                    String strClause;
                    bool bFirst = true;
                    for (auto it = idlist.begin(); it != idlist.end(); ++it)
                    {
                        if (bFirst)
                        {
                            bFirst = false;
                            strClause = itostr(*it);
                        }
                        else
                        {
                            strClause += ("," + itostr(*it));
                        }
                    }

                    String strSql = "select * from " + strTableName + " where gid=" + itostr(gid) + " and msgId in (" + strClause + ") and state=0 and created >= " + itostr(nUpdated) + " order by created desc, id desc limit 100";
                    DatabaseResult* resSet = dbConn->execQuery(strSql.c_str());
                    if (resSet)
                    {
                        while (resSet->nextRow())
                        {
                            MBCAF::Proto::MsgInfo proto;
                            Mi32 temp1;
                            resSet->getValue("msgId", temp1);
                            proto.set_msg_id(temp1);
                            resSet->getValue("userId", temp1);
                            proto.set_from_session_id(temp1);
                            resSet->getValue("created", temp1)
                            proto.set_create_time(temp1);
                            resSet->getValue("type", temp1);
                            MBCAF::Proto::MessageType mtype = MBCAF::Proto::MessageType(temp1);
                            if (MBCAFProto::MsgType_IsValid(mtype))
                            {
                                String temp;
                                resSet->getValue("content", temp)
                                proto.set_msg_type(mtype);
                                proto.set_msg_data(temp.c_str());
                                msglist.push_back(proto);
                            }
                            else
                            {
                                Mlog("invalid msgType. userId=%u, gid=%u, msgType=%u", userid, gid, mtype);
                            }
                        }
                        delete resSet;
                    }
                    else
                    {
                        Mlog("no result set for sql:%s", strSql.c_str());
                    }
                    dbMag->freeTempConnect(dbConn);
                    if (!msglist.empty())
                    {
                        M_Only(Model_Audio)->readAudios(msglist);
                    }
                }
                else
                {
                    Mlog("no db connection for gsgsslave");
                }
            }
            else
            {
                Mlog("%u is not in group:%u", userid, gid);
            }
        }
        else
        {
            Mlog("msgId is empty.");
        }
    }
    //-----------------------------------------------------------------------
    bool Model_GroupMessage::resetMessageID(uint32_t gid)
    {
        bool bRet = false;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memConn = memdbMag->getTempConnect("unread");
        if (memConn)
        {
            String strKey = "group_msg_id_" + itostr(gid);
            String strValue = "0";
            String strReply = memConn->SET(strKey, strValue);
            if (strReply == strValue)
            {
                bRet = true;
            }
            memdbMag->freeTempConnect(memConn);
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
}