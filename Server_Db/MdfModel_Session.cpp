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

#include "MdfModel_Session.h"
#include "MdfServerConnect.h"
#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfModel_Message.h"
#include "MdfModel_User.h"
#include "MdfModel_Group.h"
#include "MBCAF.MsgServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    void RecentSessionA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::RecentSessionQ proto;
        MBCAF::MsgServer::RecentSessionA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t userid = proto.user_id();
            uint32_t ltime = proto.latest_update_time();

            list<MBCAF::Proto::ContactSessionInfo> infolist;
            M_Only(Model_Session)->getRecentSession(userid, ltime, infolist);
            protoA.set_user_id(userid);
            for(auto it = infolist.begin(); it != infolist.end(); ++it)
            {
                MBCAF::Proto::ContactSessionInfo * info = protoA.add_contact_session_list();

                info->set_session_id(it->session_id());
                info->set_session_type(it->session_type());
                info->set_session_status(it->session_status());
                info->set_updated_time(it->updated_time());
                info->set_latest_msg_id(it->latest_msg_id());
                info->set_latest_msg_data(it->latest_msg_data());
                info->set_latest_msg_type(it->latest_msg_type());
                info->set_latest_msg_from_user_id(it->latest_msg_from_user_id());
            }

            Mlog("userId=%u, last_time=%u, count=%u", userid, ltime, protoA.contact_session_list_size());

            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(BubbyRecentSessionA));
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void RemoveSessionA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::RemoveSessionQ proto;
        MBCAF::MsgServer::RemoveSessionA protoA;

        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t userid = proto.user_id();
            uint32_t sessionid = proto.session_id();
            MBCAF::Proto::SessionType type = proto.session_type();
            if (MBCAF::Proto::SessionType_IsValid(type))
            {
                bool bRet = false;
                uint32_t sessionid = M_Only(Model_Session)->getSessionId(userid, sessionid, type, false);
                if (sessionid != INVALID_VALUE)
                {
                    bRet = M_Only(Model_Session)->removeSession(sessionid);
                    if (bRet)
                    {
                        M_Only(Model_User)->clearUserCounter(userid, sessionid, type);
                    }
                }
                Mlog("userId=%d, peerId=%d, result=%s", userid, sessionid, bRet ? "success" : "failed");

                protoA.set_attach_data(proto.attach_data());
                protoA.set_user_id(userid);
                protoA.set_session_id(sessionid);
                protoA.set_session_type(type);
                protoA.set_result_code(bRet ? 0 : 1);
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(BubbyRemoveSessionA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("invalied session_type. userId=%u, peerId=%u, seseionType=%u", userid, sessionid, type);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void Model_Session::getRecentSession(uint32_t userid, uint32_t lastTime, list<MBCAF::Proto::ContactSessionInfo> & infolist)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select * from MACAF_Session where userId = " + itostr(userid) + " and state = 0 and updated >" + itostr(lastTime) + " order by updated desc limit 100";

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    MBCAF::Proto::ContactSessionInfo info;
                    uint32_t temp2;
                    resSet->getValue("tid", temp2);
                    info.set_session_id(temp2);
                    info.set_session_status(::MBCAF::Proto::SessionStateType(resSet->getValue("state")));
                    resSet->getValue("type", temp2);
                    MBCAF::Proto::SessionType nSessionType = MBCAF::Proto::SessionType(temp2);
                    if (MBCAF::Proto::SessionType_IsValid(nSessionType))
                    {
                        info.set_session_type(MBCAF::Proto::SessionType(nSessionType));
                        resSet->getValue("updated", temp2);
                        info.set_updated_time(temp2);
                        infolist.push_back(info);
                    }
                    else
                    {
                        Mlog("invalid sessionType. userId=%u, peerId=%u, sessionType=%u", userid, sessionid, nSessionType);
                    }
                }
                delete resSet;
            }
            else
            {
                Mlog("no result set for sql: %s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
            if (!infolist.empty())
            {
                for (auto it = infolist.begin(); it != infolist.end();)
                {
                    uint32_t msgid = 0;
                    String msgdata;
                    MBCAF::Proto::MessageType msgtype;
                    uint32_t fromid = 0;
                    if (it->session_type() == MBCAF::Proto::ST_Single)
                    {
                        fromid = it->session_id();
                        M_Only(Model_Message)->getLastMsg(it->session_id(), userid, msgid, msgdata, msgtype);
                    }
                    else
                    {
                        M_Only(Model_GroupMessage)->getLastMsg(it->session_id(), msgid, msgdata, msgtype, fromid);
                    }
                    if (!MBCAF::Proto::MsgType_IsValid(msgtype))
                    {
                        it = infolist.erase(it);
                    }
                    else
                    {
                        it->set_latest_msg_from_user_id(fromid);
                        it->set_latest_msg_id(msgid);
                        it->set_latest_msg_data(msgdata);
                        it->set_latest_msg_type(msgtype);
                        ++it;
                    }
                }
            }
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    uint32_t Model_Session::getSessionId(uint32_t userid, uint32_t sessionid, uint32_t type, bool all)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        uint32_t sessionid = INVALID_VALUE;
        if (dbConn)
        {
            String strSql;
            if (all)
            {
                strSql = "select id from MACAF_Session where userId=" + itostr(userid) + " and tid=" + itostr(sessionid) + " and type=" + itostr(type);
            }
            else
            {
                strSql = "select id from MACAF_Session where userId=" + itostr(userid) + " and tid=" + itostr(sessionid) + " and type=" + itostr(type) + " and state=0";
            }

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    resSet->getValue("id", sessionid);
                }
                delete resSet;
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
        return sessionid;
    }
    //-----------------------------------------------------------------------
    bool Model_Session::updateSession(uint32_t sessionid, uint32_t nUpdateTime)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "update MACAF_Session set `updated`=" + itostr(nUpdateTime) + " where id=" + itostr(sessionid);
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
    bool Model_Session::removeSession(uint32_t sessionid)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            uint32_t nNow = (uint32_t)time(NULL);
            String strSql = "update MACAF_Session set state = 1, updated=" + itostr(nNow) + " where id=" + itostr(sessionid);
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
    uint32_t Model_Session::addSession(uint32_t userid, uint32_t sessionid, uint32_t type)
    {
        uint32_t sessionid = getSessionId(userid, sessionid, type, true);
        uint32_t nTimeNow = time(NULL);
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            if (INVALID_VALUE != sessionid)
            {
                String strSql = "update MACAF_Session set state=0, updated=" + itostr(nTimeNow) + " where id=" + itostr(sessionid);
                bool bRet = dbConn->exec(strSql.c_str());
                if (!bRet)
                {
                    sessionid = INVALID_VALUE;
                }
                Mlog("has relation ship set state");
            }
            else
            {
                String strSql = "insert into MACAF_Session (`userId`,`tid`,`type`,`state`,`created`,`updated`) values(?,?,?,?,?,?)";
                PrepareExec * stmt = new PrepareExec();
                if (stmt->prepare(dbConn->getConnect(), strSql))
                {
                    uint32_t mState = 0;
                    uint32_t index = 0;
                    stmt->setParam(index++, userid);
                    stmt->setParam(index++, sessionid);
                    stmt->setParam(index++, type);
                    stmt->setParam(index++, mState);
                    stmt->setParam(index++, nTimeNow);
                    stmt->setParam(index++, nTimeNow);
                    bool bRet = stmt->exec();
                    if (bRet)
                    {
                        sessionid = dbConn->getInsertId();
                    }
                    else
                    {
                        Mlog("insert message failed. %s", strSql.c_str());
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
        return sessionid;
    }
    //-----------------------------------------------------------------------
}








