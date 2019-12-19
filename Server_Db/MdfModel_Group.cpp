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

#include "MdfModel_Group.h"
#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfServerConnect.h"
#include "MdfModel_Audio.h"
#include "MdfModel_User.h"
#include "MdfModel_Session.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.Proto.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    void GroupCreateA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupCreateQ proto;
        MBCAF::MsgServer::GroupCreateA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;
            uint32_t userid = proto.user_id();
            String gname = proto.group_name();
            MBCAF::Proto::GroupType gtype = proto.group_type();
            if (MBCAF::Proto::GroupType_IsValid(gtype))
            {
                set<uint32_t> oplist;
                String gavatar = proto.group_avatar();
                uint32_t gmemberCnt = proto.member_id_list_size();
                for (uint32_t i = 0; i < gmemberCnt; ++i)
                {
                    uint32_t userid = proto.member_id_list(i);
                    oplist.insert(userid);
                }
                Mlog("GroupCreateA.%d create %s, userCnt=%u", userid, gname.c_str(), oplist.size());

                uint32_t gid = M_Only(Model_Group)->createGroup(userid, gname, gavatar, gtype, oplist);
                protoA.set_user_id(userid);
                protoA.set_group_name(gname);
                for (auto it = oplist.begin(); it != oplist.end(); ++it)
                {
                    protoA.add_user_id_list(*it);
                }
                if (gid != INVALID_VALUE)
                {
                    protoA.set_result_code(0);
                    protoA.set_group_id(gid);
                }
                else
                {
                    protoA.set_result_code(1);
                }

                Mlog("GroupCreateA.%d create %s, userCnt=%u, result:%d", userid, gname.c_str(), oplist.size(), protoA.result_code());

                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(CreateGroupA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("invalid group type.userId=%u, groupType=%u, groupName=%s", userid, gtype, gname.c_str());
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void GroupListA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupListQ proto;
        MBCAF::MsgServer::GroupListA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;
            list<MBCAF::Proto::GroupVersionInfo> gverlist;
            uint32_t userid = proto.user_id();
            M_Only(Model_Group)->getUserGroup(userid, gverlist, MBCAF::Proto::GROUP_TYPE_NORMAL);
            protoA.set_user_id(userid);
            for (auto it = gverlist.begin(); it != gverlist.end(); ++it)
            {
                MBCAF::Proto::GroupVersionInfo * tempinfo = protoA.add_group_version_list();
                tempinfo->set_group_id(it->group_id());
                tempinfo->set_version(it->version());
            }

            Mlog("GroupListA. userId=%u, count=%d", userid, protoA.group_version_list_size());

            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(GroupListA0);
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void GroupInfoListA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupInfoListQ proto;
        MBCAF::MsgServer::GroupInfoListA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;
            uint32_t userid = proto.user_id();
            uint32_t nGroupCnt = proto.group_version_list_size();

            map<uint32_t, MBCAF::Proto::GroupVersionInfo> verlist;
            for (uint32_t i = 0; i < nGroupCnt; ++i)
            {
                MBCAF::Proto::GroupVersionInfo groupInfo = proto.group_version_list(i);
                if (M_Only(Model_Group)->isValidateGroupId(groupInfo.group_id()))
                {
                    verlist[groupInfo.group_id()] = groupInfo;
                }
            }
            list<MBCAF::Proto::GroupInfo> infolist;
            M_Only(Model_Group)->getGroupInfo(verlist, infolist);

            protoA.set_user_id(userid);
            for (auto it = infolist.begin(); it != infolist.end(); ++it)
            {
                MBCAF::Proto::GroupInfo* pGroupInfo = protoA.add_group_info_list();
                pGroupInfo->set_group_id(it->group_id());
                pGroupInfo->set_version(it->version());
                pGroupInfo->set_group_name(it->group_name());
                pGroupInfo->set_group_avatar(it->group_avatar());
                pGroupInfo->set_group_creator_id(it->group_creator_id());
                pGroupInfo->set_group_type(it->group_type());
                pGroupInfo->set_shield_status(it->shield_status());
                uint32_t nGroupMemberCnt = it->group_member_list_size();
                for (uint32_t i = 0; i < nGroupMemberCnt; ++i) 
                {
                    uint32_t userId = it->group_member_list(i);
                    pGroupInfo->add_group_member_list(userId);
                }
            }

            Mlog("userId=%u, requestCount=%u", userid, nGroupCnt);

            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(GroupInfoA));
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void GroupMemberSA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupMemberSQ proto;
        MBCAF::MsgServer::GroupMemberSA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t userid = proto.user_id();
            uint32_t gid = proto.group_id();
            MBCAF::Proto::GroupModifyType nType = proto.change_type();
            if (MBCAF::Proto::GroupModifyType_IsValid(nType) &&
                M_Only(Model_Group)->isValidateGroupId(gid))
            {
                MdfMessage * remsg = new MdfMessage;
                uint32_t nCnt = proto.member_id_list_size();
                set<uint32_t> setUserId;
                for (uint32_t i = 0; i < nCnt; ++i)
                {
                    setUserId.insert(proto.member_id_list(i));
                }
                list<uint32_t> useridlist;
                bool bRet = M_Only(Model_Group)->modifyGroupMember(userid, gid, nType, setUserId, useridlist);
                protoA.set_user_id(userid);
                protoA.set_group_id(gid);
                protoA.set_change_type(nType);
                protoA.set_result_code(bRet ? 0 : 1);
                if (bRet)
                {
                    for (auto it = setUserId.begin(); it != setUserId.end(); ++it)
                    {
                        protoA.add_chg_user_id_list(*it);
                    }

                    for (auto it = useridlist.begin(); it != useridlist.end(); ++it)
                    {
                        protoA.add_cur_user_id_list(*it);
                    }
                }
                Mlog("userId=%u, gid=%u, result=%u, changeCount:%u, currentCount=%u", userid, gid, bRet ? 0 : 1, protoA.chg_user_id_list_size(), protoA.cur_user_id_list_size());
                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(GroupMemberSA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("invalid groupModifyType or gid. userId=%u, gid=%u, groupModifyType=%u", userid, gid, nType);
            }

        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void GroupShieldSA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::GroupShieldSQ proto;
        MBCAF::MsgServer::GroupShieldSA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t userid = proto.user_id();
            uint32_t gid = proto.group_id();
            uint32_t mState = proto.shield_status();
            if (M_Only(Model_Group)->isValidateGroupId(gid))
            {
                MdfMessage * remsg = new MdfMessage;
                bool bRet = M_Only(Model_Group)->setPush(userid, gid, IM_GROUP_SETTING_PUSH, mState);
                protoA.set_user_id(userid);
                protoA.set_group_id(gid);
                protoA.set_result_code(bRet ? 0 : 1);

                Mlog("userId=%u, gid=%u, result=%u", userid, gid, protoA.result_code());

                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(MSMSG(GroupShieldSA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("Invalid group.userId=%u, gid=%u", userid, gid);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void GroupShieldA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::ServerBase::GroupShieldQ proto;
        MBCAF::ServerBase::GroupShieldA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t gid = proto.group_id();
            uint32_t nUserCnt = proto.user_id_size();
            if (M_Only(Model_Group)->isValidateGroupId(gid))
            {
                MdfMessage * remsg = new MdfMessage;
                list<uint32_t> usridlist;
                for (uint32_t i = 0; i < nUserCnt; ++i)
                {
                    usridlist.push_back(proto.user_id(i));
                }
                list<MBCAF::Proto::ShieldStatus> statelist;
                M_Only(Model_Group)->getPush(gid, usridlist, statelist);

                protoA.set_group_id(gid);
                for (auto it = statelist.begin(); it != statelist.end(); ++it)
                {
                    MBCAF::Proto::ShieldStatus* pStatus = protoA.add_shield_status_list();
                    pStatus->set_user_id(it->user_id());
                    pStatus->set_group_id(it->group_id());
                    pStatus->set_shield_status(it->shield_status());
                }

                Mlog("gid=%u, count=%u", gid, nUserCnt);

                protoA.set_attach_data(proto.attach_data());
                remsg->setProto(&protoA);
                remsg->setSeqIdx(msg->getSeqIdx());
                remsg->setCommandID(SBMSG(GroupShieldA));
                M_Only(DataSyncManager)->response(qconn, remsg);
            }
            else
            {
                Mlog("Invalid gid. gid=%u", gid);
            }
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_Group);
    //-----------------------------------------------------------------------
    Model_Group::Model_Group()
    {
    }
    //-----------------------------------------------------------------------
    Model_Group::~Model_Group()
    {
    }
    //-----------------------------------------------------------------------
    uint32_t Model_Group::createGroup(uint32_t userid, const String & gname,
        const String & gavatar, uint32_t gtype, set<uint32_t> & oplist)
    {
        uint32_t gid = INVALID_VALUE;
        do
        {
            if (gname.empty())
            {
                break;
            }
            if (oplist.empty())
            {
                break;
            }
            if (!insertNewGroup(userid, gname, gavatar, gtype, (uint32_t)oplist.size(), gid)) 
            {
                break;
            }
            bool bRet = M_Only(Model_GroupMessage)->resetMessageID(gid);
            if (!bRet)
            {
                Mlog("reset msgId failed. gid=%u", gid);
            }

            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
            if (dbConn)
            {
                String strSql = "delete from MACAF_GroupMember where gid=" + itostr(gid);
                dbConn->exec(strSql.c_str());
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsmaster");
            }
            MembaseManager * memdbMag = M_Only(MembaseManager);
            MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
            if (memdbConn)
            {
                String strKey = "group_member_" + itostr(gid);
                map<String, String> mapRet;
                bool bRet = memdbConn->HGETALL(strKey, mapRet);
                if (bRet)
                {
                    for (auto it = mapRet.begin(); it != mapRet.end(); ++it)
                    {
                        memdbConn->HDEL(strKey, it->first);
                    }
                }
                else
                {
                    Mlog("hgetall %s failed", strKey.c_str());
                }
                memdbMag->freeTempConnect(memdbConn);
            }
            else
            {
                Mlog("no cache connection for group_member");
            }
            insertNewMember(gid, oplist);

        } while (false);
        return gid;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::removeGroup(uint32_t userid, uint32_t gid, list<uint32_t> & useridlist)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        set<uint32_t> setGroupUsers;
        if (dbConn)
        {
            String strSql = "select creator from MACAF_Group where id=" + itostr(gid);
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                uint32_t nCreator;
                while (resSet->nextRow())
                {
                    resSet->getValue("creator", nCreator);
                }

                if (0 == nCreator || nCreator == userid)
                {
                    strSql = "update MACAF_Group set state=0 where id=" + itostr(gid);
                    bRet = dbConn->exec(strSql.c_str());
                }
                delete  resSet;
            }

            if (bRet)
            {
                strSql = "select userId from MACAF_GroupMember where gid=" + itostr(gid);
                DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
                if (resSet)
                {
                    while (resSet->nextRow())
                    {
                        uint32_t mID;
                        resSet->getValue("userId", mID);
                        setGroupUsers.insert(mID);
                    }
                    delete resSet;
                }
            }
            dbMag->freeTempConnect(dbConn);
        }

        if (bRet)
        {
            bRet = removeMember(gid, setGroupUsers, useridlist);
        }

        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_Group::getUserGroup(uint32_t userid, list<MBCAF::Proto::GroupVersionInfo> & gverlist, uint32_t gtype)
    {
        list<uint32_t> idlist;
        getUserGroup(userid, idlist, 0);
        if (idlist.size() != 0)
        {
            getGroupVersion(idlist, gverlist, gtype);
        }
    }
    //-----------------------------------------------------------------------
    void Model_Group::getGroupInfo(map<uint32_t, MBCAF::Proto::GroupVersionInfo> & verlist,
        list<MBCAF::Proto::GroupInfo> & infolist)
    {
        if (!verlist.empty())
        {
            DatabaseManager* dbMag = M_Only(DatabaseManager);
            DatabaseConnect* dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                String strClause;
                bool bFirst = true;
                for (auto it = verlist.begin(); it != verlist.end(); ++it)
                {
                    if (bFirst)
                    {
                        bFirst = false;
                        strClause = itostr(it->first);
                    }
                    else
                    {
                        strClause += ("," + itostr(it->first));
                    }
                }
                String strSql = "select * from MACAF_Group where id in (" + strClause + ") order by updated desc";
                DatabaseResult* resSet = dbConn->execQuery(strSql.c_str());
                if (resSet)
                {
                    while (resSet->nextRow())
                    {
                        uint32_t gid;
                        uint32_t nVersion;
                        resSet->getValue("id", gid);
                        resSet->getValue("version", nVersion);
                        if (verlist[gid].version() < nVersion)
                        {
                            String temp;
                            MBCAF::Proto::GroupInfo cGroupInfo;
                            cGroupInfo.set_group_id(gid);
                            cGroupInfo.set_version(nVersion);
                            resSet->getValue("name", temp);
                            cGroupInfo.set_group_name(temp.c_str());
                            resSet->getValue("avatar", temp);
                            cGroupInfo.set_group_avatar(temp.c_str());
                            Mi32 temp1;
                            resSet->getValue("type", temp1);
                            MBCAF::Proto::GroupType gtype = MBCAF::Proto::GroupType(temp1);
                            if (MBCAF::Proto::GroupType_IsValid(gtype))
                            {
                                cGroupInfo.set_group_type(gtype);
                                resSet->getValue("creator", temp1);
                                cGroupInfo.set_group_creator_id(temp1);
                                infolist.push_back(cGroupInfo);
                            }
                            else
                            {
                                Mlog("invalid groupType. gid=%u, groupType=%u", gid, gtype);
                            }
                        }
                    }
                    delete resSet;
                }
                else
                {
                    Mlog("no result set for sql:%s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);
                if (!infolist.empty())
                {
                    fillGroupMember(infolist);
                }
            }
            else
            {
                Mlog("no db connection for gsgsslave");
            }
        }
        else
        {
            Mlog("no ids in map");
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Group::modifyGroupMember(uint32_t userid, uint32_t gid,
        MBCAF::Proto::GroupModifyType type, set<uint32_t> & setUserId,
            list<uint32_t> & useridlist)
    {
        bool bRet = false;
        if (hasModifyPermission(userid, gid, type))
        {
            switch (type)
            {
            case MBCAF::Proto::GMT_Add:
                bRet = addMember(gid, setUserId, useridlist);
                break;
            case MBCAF::Proto::GMT_delete:
                bRet = removeMember(gid, setUserId, useridlist);
                removeSession(gid, setUserId);
                break;
            default:
                Mlog("unknown type:%u while modify group.%u->%u", type, userid, gid);
                break;
            }
            //if modify group member success, need to inc the group version and clear the user count;
            if (bRet)
            {
                DatabaseManager * dbMag = M_Only(DatabaseManager);
                DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
                if (dbConn)
                {
                    String strSql = "update MACAF_Group set version=version+1 where id=" + itostr(gid);
                    dbConn->exec(strSql.c_str())
                    dbMag->freeTempConnect(dbConn);
                }
                else
                {
                    Mlog("no db connection for gsgsmaster");
                }
                for (auto it = setUserId.begin(); it != setUserId.end(); ++it)
                {
                    uint32_t userid = *it;
                    M_Only(Model_User)->clearUserCounter(userid, gid, MBCAF::Proto::ST_Group);
                }
            }
        }
        else
        {
            Mlog("user:%u has no permission to modify group:%u", userid, gid);
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::insertNewGroup(uint32_t userid, const String & gname,
        const String & gavatar, uint32_t gtype, uint32_t gmemberCnt,
        uint32_t & gid)
    {
        bool bRet = false;
        gid = INVALID_VALUE;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "insert into MACAF_Group(`name`, `avatar`, `creator`, `type`,`userCnt`, `state`, `version`, `lastChated`, `updated`, `created`) "\
                "values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

            PrepareExec* pStmt = new PrepareExec();
            if (pStmt->prepare(dbConn->getConnect(), strSql))
            {
                uint32_t nCreated = (uint32_t)time(NULL);
                uint32_t index = 0;
                uint32_t mState = 0;
                uint32_t nVersion = 1;
                uint32_t nLastChat = 0;
                pStmt->setParam(index++, gname);
                pStmt->setParam(index++, gavatar);
                pStmt->setParam(index++, userid);
                pStmt->setParam(index++, gtype);
                pStmt->setParam(index++, gmemberCnt);
                pStmt->setParam(index++, mState);
                pStmt->setParam(index++, nVersion);
                pStmt->setParam(index++, nLastChat);
                pStmt->setParam(index++, nCreated);
                pStmt->setParam(index++, nCreated);

                bRet = pStmt->exec();
                if (bRet)
                {
                    gid = pStmt->getInsertId();
                }
            }
            delete pStmt;
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::insertNewMember(uint32_t gid, set<uint32_t> & setUsers)
    {
        bool bRet = false;
        uint32_t nUserCnt = (uint32_t)setUsers.size();
        if (gid != INVALID_VALUE && nUserCnt > 0)
        {
            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                uint32_t nCreated = (uint32_t)time(NULL);
                String strClause;
                bool bFirst = true;
                for (auto it = setUsers.begin(); it != setUsers.end(); ++it)
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
                String strSql = "select userId from MACAF_GroupMember where gid=" + itostr(gid) + " and userId in (" + strClause + ")";
                DatabaseResult* pResult = dbConn->execQuery(strSql.c_str());
                set<uint32_t> setHasUser;
                if (pResult)
                {
                    while (pResult->nextRow())
                    {
                        Mi32 temp;
                        pResult->getValue("userId", temp);
                        setHasUser.insert(temp);
                    }
                    delete pResult;
                }
                else
                {
                    Mlog("no result for sql:%s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);

                dbConn = dbMag->getTempConnect("gsgsmaster");
                if (dbConn)
                {
                    MembaseManager* memdbMag = M_Only(MembaseManager);
                    MembaseConnect* memdbConn = memdbMag->getTempConnect("group_member");
                    if (memdbConn)
                    {
                        // 设置已经存在群中人的状态
                        if (!setHasUser.empty())
                        {
                            strClause.clear();
                            bFirst = true;
                            for (auto it = setHasUser.begin(); it != setHasUser.end(); ++it)
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

                            strSql = "update MACAF_GroupMember set state=0, updated=" + itostr(nCreated) + " where gid=" + itostr(gid) + " and userId in (" + strClause + ")";
                            dbConn->exec(strSql.c_str());
                        }
                        strSql = "insert into MACAF_GroupMember(`gid`, `userId`, `state`, `created`, `updated`) values(?,?,?,?,?)";

                        //插入新成员
                        auto it = setUsers.begin();
                        uint32_t mState = 0;
                        uint32_t nIncMemberCnt = 0;
                        for (; it != setUsers.end();)
                        {
                            uint32_t userid = *it;
                            if (setHasUser.find(userid) == setHasUser.end())
                            {
                                PrepareExec* pStmt = new PrepareExec();
                                if (pStmt->prepare(dbConn->getConnect(), strSql))
                                {
                                    uint32_t index = 0;
                                    pStmt->setParam(index++, gid);
                                    pStmt->setParam(index++, userid);
                                    pStmt->setParam(index++, mState);
                                    pStmt->setParam(index++, nCreated);
                                    pStmt->setParam(index++, nCreated);
                                    pStmt->exec();
                                    ++nIncMemberCnt;
                                    delete pStmt;
                                }
                                else
                                {
                                    setUsers.erase(it++);
                                    delete pStmt;
                                    continue;
                                }
                            }
                            ++it;
                        }
                        if (nIncMemberCnt != 0)
                        {
                            strSql = "update MACAF_Group set userCnt=userCnt+" + itostr(nIncMemberCnt) + " where id=" + itostr(gid);
                            dbConn->exec(strSql.c_str());
                        }

                        //更新一份到redis中
                        String strKey = "group_member_" + itostr(gid);
                        for (auto it = setUsers.begin(); it != setUsers.end(); ++it)
                        {
                            memdbConn->HSET(strKey, itostr(*it), itostr(nCreated));
                        }
                        memdbMag->freeTempConnect(memdbConn);
                        bRet = true;
                    }
                    else
                    {
                        Mlog("no cache connection");
                    }
                    dbMag->freeTempConnect(dbConn);
                }
                else
                {
                    Mlog("no db connection for gsgsmaster");
                }
            }
            else
            {
                Mlog("no db connection for gsgsslave");
            }
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_Group::getUserGroup(uint32_t userid, list<uint32_t>& idlist, uint32_t cnt)
    {
        DatabaseManager* dbMag = M_Only(DatabaseManager);
        DatabaseConnect* dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql;
            if (cnt != 0)
            {
                strSql = "select gid from MACAF_GroupMember where userId=" + itostr(userid) + " and state = 0 order by updated desc, id desc limit " + itostr(cnt);
            }
            else
            {
                strSql = "select gid from MACAF_GroupMember where userId=" + itostr(userid) + " and state = 0 order by updated desc, id desc";
            }

            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    uint32_t gid;
                    resSet->getValue("gid", gid);
                    idlist.push_back(gid);
                }
                delete resSet;
            }
            else
            {
                Mlog("no result set for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    void Model_Group::getGroupVersion(list<uint32_t> &idlist, list<MBCAF::Proto::GroupVersionInfo> &gverlist, uint32_t gtype)
    {
        if (!idlist.empty())
        {
            DatabaseManager* dbMag = M_Only(DatabaseManager);
            DatabaseConnect* dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                String strClause;
                bool bFirst = true;
                for (list<uint32_t>::iterator it = idlist.begin(); it != idlist.end(); ++it)
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

                String strSql = "select id,version from MACAF_Group where id in (" + strClause + ")";
                if (0 != gtype)
                {
                    strSql += " and type=" + itostr(gtype);
                }
                strSql += " order by updated desc";

                DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
                if (resSet)
                {
                    while (resSet->nextRow())
                    {
                        Mi32 temp;
                        MBCAF::Proto::GroupVersionInfo group;
                        resSet->getValue("id", temp)
                        group.set_group_id(temp);
                        resSet->getValue("version", temp)
                        group.set_version(temp);
                        gverlist.push_back(group);
                    }
                    delete resSet;
                }
                else
                {
                    Mlog("no result set for sql:%s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsslave");
            }
        }
        else
        {
            Mlog("group ids is empty");
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Group::isInGroup(uint32_t userid, uint32_t gid)
    {
        bool bRet = false;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
        if (memdbConn)
        {
            String strKey = "group_member_" + itostr(gid);
            String strField = itostr(userid);
            String strValue = memdbConn->HGET(strKey, strField);
            memdbMag->freeTempConnect(memdbConn);
            if (!strValue.empty())
            {
                bRet = true;
            }
        }
        else
        {
            Mlog("no cache connection for group_member");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::hasModifyPermission(uint32_t userid, uint32_t gid, MBCAF::Proto::GroupModifyType type)
    {
        if (userid == 0)
        {
            return true;
        }

        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select creator, type from MACAF_Group where id=" + itostr(gid);
            DatabaseResult* resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    uint32_t nCreator, temp2;
                    resSet->getValue("creator", nCreator);
                    resSet->getValue("type", temp2);
                    MBCAF::Proto::GroupType gtype = MBCAF::Proto::GroupType(temp2);
                    if (MBCAF::Proto::GroupType_IsValid(gtype))
                    {
                        if (MBCAF::Proto::GROUP_TYPE_TMP == gtype && MBCAF::Proto::GMT_Add == type)
                        {
                            bRet = true;
                            break;
                        }
                        else
                        {
                            if (nCreator == userid)
                            {
                                bRet = true;
                                break;
                            }
                        }
                    }
                }
                delete resSet;
            }
            else
            {
                Mlog("no result for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::addMember(uint32_t gid, set<uint32_t> & oplist, list<uint32_t> & useridlist)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
        if (memdbConn)
        {
            String strKey = "group_member_" + itostr(gid);
            for (auto it = oplist.begin(); it != oplist.end();)
            {
                String strField = itostr(*it);
                String strValue = memdbConn->HGET(strKey, strField);
                memdbMag->freeTempConnect(memdbConn);
                if (!strValue.empty())
                {
                    oplist.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            Mlog("no cache connection for group_member");
        }
        bool bRet = insertNewMember(gid, oplist);
        getGroupUser(gid, useridlist);
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::removeMember(uint32_t gid, set<uint32_t> & oplist, list<uint32_t> & useridlist)
    {
        if (oplist.size() <= 0)
        {
            return true;
        }
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            MembaseManager * memdbMag = M_Only(MembaseManager);
            MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
            if (memdbConn)
            {
                String strClause;
                bool bFirst = true;
                for (auto it = oplist.begin(); it != oplist.end(); ++it)
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
                String strSql = "update MACAF_GroupMember set state=1 where  gid =" + itostr(gid) + " and userId in(" + strClause + ")";
                dbConn->exec(strSql.c_str());

                String strKey = "group_member_" + itostr(gid);
                for (auto it = oplist.begin(); it != oplist.end(); ++it)
                {
                    String strField = itostr(*it);
                    memdbConn->HDEL(strKey, strField);
                }
                memdbMag->freeTempConnect(memdbConn);
                bRet = true;
            }
            else
            {
                Mlog("no cache connection");
            }
            dbMag->freeTempConnect(dbConn);
            if (bRet)
            {
                getGroupUser(gid, useridlist);
            }
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    bool Model_Group::setPush(uint32_t userid, uint32_t gid, uint32_t type, uint32_t mState)
    {
        bool bRet = false;
        if(!isInGroup(userid, gid))
        {
            Mlog("user:%d is not in group:%d", userid, gid);
            return bRet;;
        }

        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_set");
        if (memdbConn)
        {
            String strGroupKey = "group_set_" + itostr(gid);
            String strField = itostr(userid) + "_" + itostr(type);
            int nRet = memdbConn->HSET(strGroupKey, strField, itostr(mState));
            memdbMag->freeTempConnect(memdbConn);
            if (nRet != -1)
            {
                bRet = true;
            }
        }
        else
        {
            Mlog("no cache connection for group_set");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_Group::getPush(uint32_t gid, list<uint32_t>& usridlist, list<MBCAF::Proto::ShieldStatus> & statelist)
    {
        if (usridlist.empty())
        {
            return;
        }
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_set");
        if (memdbConn)
        {
            String strGroupKey = "group_set_" + itostr(gid);
            map<String, String> mapResult;
            bool bRet = memdbConn->HGETALL(strGroupKey, mapResult);
            memdbMag->freeTempConnect(memdbConn);
            if (bRet)
            {
                for (auto it = usridlist.begin(); it != usridlist.end(); ++it)
                {
                    String strField = itostr(*it) + "_" + itostr(IM_GROUP_SETTING_PUSH);
                    auto itResult = mapResult.find(strField);
                    MBCAF::Proto::ShieldStatus state;
                    state.set_group_id(gid);
                    state.set_user_id(*it);
                    if (itResult != mapResult.end())
                    {
                        state.set_shield_status(strtoi(itResult->second));
                    }
                    else
                    {
                        state.set_shield_status(0);
                    }
                    statelist.push_back(state);
                }
            }
            else
            {
                Mlog("hgetall %s failed!", strGroupKey.c_str());
            }
        }
        else
        {
            Mlog("no cache connection for group_set");
        }
    }
    //-----------------------------------------------------------------------
    void Model_Group::getGroupUser(uint32_t gid, list<uint32_t> & usridlist)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
        if (memdbConn)
        {
            String strKey = "group_member_" + itostr(gid);
            map<String, String> templist;
            bool bRet = memdbConn->HGETALL(strKey, templist);
            memdbMag->freeTempConnect(memdbConn);
            if (bRet)
            {
                for (auto it = templist.begin(); it != templist.end(); ++it)
                {
                    uint32_t userid = strtoi(it->first);
                    usridlist.push_back(userid);
                }
            }
            else
            {
                Mlog("hgetall %s failed!", strKey.c_str());
            }
        }
        else
        {
            Mlog("no cache connection for group_member");
        }
    }
    //-----------------------------------------------------------------------
    void Model_Group::updateGroupChat(uint32_t gid)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "update MACAF_Group set lastChated=" + itostr((uint32_t)time(NULL)) + " where id=" + itostr(gid);
            dbConn->exec(strSql.c_str());
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Group::isValidateGroupId(uint32_t gid)
    {
        bool bRet = false;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
        if (memdbConn)
        {
            String strKey = "group_member_" + itostr(gid);
            bRet = memdbConn->EXISTS(strKey);
            memdbMag->freeTempConnect(memdbConn);
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    void Model_Group::removeSession(uint32_t gid, const set<uint32_t> & oplist)
    {
        for (auto it = oplist.begin(); it != oplist.end(); ++it)
        {
            uint32_t userid = *it;
            uint32_t nSessionId = M_Only(Model_Session)->getSessionId(userid, gid, MBCAF::Proto::ST_Group, false);
            M_Only(Model_Session)->removeSession(nSessionId);
        }
    }
    //-----------------------------------------------------------------------
    void Model_Group::fillGroupMember(list<MBCAF::Proto::GroupInfo> & infolist)
    {
        for (auto it = infolist.begin(); it != infolist.end(); ++it)
        {
            list<uint32_t> lsUserIds;
            uint32_t gid = it->group_id();
            getGroupUser(gid, lsUserIds);
            for (auto itUserId = lsUserIds.begin(); itUserId != lsUserIds.end(); ++itUserId)
            {
                it->add_group_member_list(*itUserId);
            }
        }
    }
    //-----------------------------------------------------------------------
    uint32_t Model_Group::getUserJoinTime(uint32_t gid, uint32_t userid)
    {
        uint32_t nTime = 0;
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("group_member");
        if (memdbConn)
        {
            String strKey = "group_member_" + itostr(gid);
            String strField = itostr(userid);
            String strValue = memdbConn->HGET(strKey, strField);
            memdbMag->freeTempConnect(memdbConn);
            if (!strValue.empty())
            {
                nTime = strtoi(strValue);
            }
        }
        else
        {
            Mlog("no cache connection for group_member");
        }
        return  nTime;
    }
    //-----------------------------------------------------------------------
}