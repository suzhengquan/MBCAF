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

#include "MdfModel_User.h"
#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfDataSyncManager.h"
#include "MdfServerConnect.h"
#include "MdfHttpClient.h"
#include "MdfModel_Depart.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.MsgServer.pb.h"
#include "MBCAF.Proto.pb.h"
#include "MdfEncrypt.h"
#include "json/json.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //----------------------------------------------------------------
    bool LoginFunc(const String & name, const String & strPass, MBCAF::Proto::UserInfo & user)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select * from MACAF_User where name='" + name + "' and state=0";
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                String strResult, strSalt;
                uint32_t id, gender, departmentID, state;
                String nickname, avatarFile, email, realName, tel, domain, signInfo;
                while (resSet->nextRow())
                {
                    resSet->getValue("id", id);
                    resSet->getValue("password", strResult);
                    resSet->getValue("salt", strSalt);
                    resSet->getValue("nick", nickname);
                    resSet->getValue("sex" gender);
                    resSet->getValue("name", realName);
                    resSet->getValue("domain", domain);
                    resSet->getValue("phone", tel);
                    resSet->getValue("email", email);
                    resSet->getValue("avatar", avatarFile);
                    resSet->getValue("departId", departmentID);
                    resSet->getValue("state", state);
                    resSet->getValue("sign_info", signInfo);
                }

                String strInPass = strPass + strSalt;
                char szMd5[33];
                MD5::calc(strInPass.c_str(), strInPass.length(), szMd5);
                String strOutPass(szMd5);

                //if(strOutPass == strResult)
                {
                    bRet = true;
                    user.set_user_id(id);
                    user.set_user_nick_name(nickname);
                    user.set_user_gender(gender);
                    user.set_user_real_name(realName);
                    user.set_user_domain(domain);
                    user.set_user_tel(tel);
                    user.set_email(email);
                    user.set_avatar_url(avatarFile);
                    user.set_department_id(departmentID);
                    user.set_status(state);
                    user.set_sign_info(signInfo);
                }
                delete resSet;
            }
            dbMag->freeTempConnect(dbConn);
        }
        return bRet;
    }
    //----------------------------------------------------------------
    hash_map<String, list<uint32_t> > g_hmLimits;
    ACE_Thread_Mutex g_cLimitLock;
    //----------------------------------------------------------------
    void UserInfoListA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::UserInfoListQ proto;
        MBCAF::MsgServer::UserInfoListA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t fromid = proto.user_id();
            uint32_t usercnt = proto.user_id_list_size();
            std::list<uint32_t> idList;
            for (uint32_t i = 0; i < usercnt; ++i)
            {
                idList.push_back(proto.user_id_list(i));
            }
            std::list<MBCAF::Proto::UserInfo> templist;
            M_Only(Model_User)->getUsers(idList, templist);
            protoA.set_user_id(fromid);
            for (list<MBCAF::Proto::UserInfo>::iterator it = templist.begin(); it != templist.end(); ++it)
            {
                MBCAF::Proto::UserInfo * info = protoA.add_user_info_list();

                info->set_user_id(it->user_id());
                info->set_user_gender(it->user_gender());
                info->set_user_nick_name(it->user_nick_name());
                info->set_avatar_url(it->avatar_url());
                info->set_sign_info(it->sign_info());
                info->set_department_id(it->department_id());
                info->set_email(it->email());
                info->set_user_real_name(it->user_real_name());
                info->set_user_tel(it->user_tel());
                info->set_user_domain(it->user_domain());
                info->set_status(it->status());
            }
            Mlog("userId=%u, userCnt=%u", fromid, usercnt);
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(BubbyObjectInfoA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //----------------------------------------------------------------
    void VaryUserInfoListA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryUserInfoListQ proto;
        MBCAF::MsgServer::VaryUserInfoListA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t qid = proto.user_id();
            uint32_t lasttime = proto.latest_update_time();
            uint32_t nLastUpdate = M_Only(DataSyncManager)->getUserUpdate();

            list<MBCAF::Proto::UserInfo> templist;
            if (nLastUpdate > lasttime)
            {
                list<uint32_t> idlist;
                M_Only(Model_User)->getChangedId(lasttime, idlist);
                M_Only(Model_User)->getUsers(idlist, templist);
            }
            protoA.set_user_id(qid);
            protoA.set_latest_update_time(lasttime);
            for (list<MBCAF::Proto::UserInfo>::iterator it = templist.begin();
                it != templist.end(); ++it)
            {
                MBCAF::Proto::UserInfo * info = protoA.add_user_list();

                info->set_user_id(it->user_id());
                info->set_user_gender(it->user_gender());
                info->set_user_nick_name(it->user_nick_name());
                info->set_avatar_url(it->avatar_url());
                info->set_sign_info(it->sign_info());
                info->set_department_id(it->department_id());
                info->set_email(it->email());
                info->set_user_real_name(it->user_real_name());
                info->set_user_tel(it->user_tel());
                info->set_user_domain(it->user_domain());
                info->set_status(it->status());
            }
            Mlog("userId=%u,nLastUpdate=%u, last_time=%u, userCnt=%u", qid, nLastUpdate, lasttime, protoA.user_list_size());
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(BuddyObjectListA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //----------------------------------------------------------------
    void SignInfoSA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::SignInfoSQ proto;
        MBCAF::MsgServer::SignInfoSA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t qid = proto.user_id();
            const String & signtxt = proto.sign_info();

            bool result = M_Only(Model_User)->updateUserSignInfo(qid, signtxt);

            protoA.set_user_id(qid);
            protoA.set_result_code(result ? 0 : 1);
            if (result)
            {
                protoA.set_sign_info(signtxt);
                Mlog("SignInfoSA sucess, user_id=%u, sign_info=%s", qid, signtxt.c_str());
            }
            else
            {
                Mlog("SignInfoSA false, user_id=%u, sign_info=%s", qid, signtxt.c_str());
            }

            MdfMessage * remsg = new MdfMessage();
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(BuddyChangeSignatureA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("SignInfoSA: SignInfoSQ ParseFromArray failed!!!");
        }
    }
    //----------------------------------------------------------------
    void PushShieldSA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::ServerBase::PushShieldSQ proto;
        MBCAF::ServerBase::PushShieldSA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t qid = proto.user_id();
            uint32_t state = proto.shield_status();

            bool result = M_Only(Model_User)->updatePushShield(qid, state);

            protoA.set_user_id(qid);
            protoA.set_result_code(result ? 0 : 1);
            if (result)
            {
                protoA.set_shield_status(state);
                Mlog("PushShieldSA sucess, user_id=%u, shield_status=%u", qid, state);
            }
            else
            {
                Mlog("PushShieldSA false, user_id=%u, shield_status=%u", qid, state);
            }

            MdfMessage * remsg = new MdfMessage();
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(SBMSG(PushShieldSA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("PushShieldSA: PushShieldSQ ParseFromArray failed!!!");
        }
    }
    //----------------------------------------------------------------
    void PushShieldA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::ServerBase::PushShieldQ proto;
        MBCAF::ServerBase::PushShieldA protoA;
        if (msg->toProto(&proto))
        {
            uint32_t qid = proto.user_id();
            uint32_t state = 0;

            bool result = M_Only(Model_User)->getPushShield(qid, &state);

            protoA.set_user_id(qid);
            protoA.set_result_code(result ? 0 : 1);
            if (result)
            {
                protoA.set_shield_status(state);
                Mlog("PushShieldA sucess, user_id=%u, shield_status=%u", qid, state);
            }
            else
            {
                Mlog("PushShieldA false, user_id=%u", qid);
            }

            MdfMessage * remsg = new MdfMessage();
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(SBMSG(PushShieldA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("PushShieldA: PushShieldQ ParseFromArray failed!!!");
        }
    }
    //----------------------------------------------------------------
    void VaryDepartListA(ServerConnect * sconn, MdfMessage * msg)
    {
        MBCAF::MsgServer::VaryDepartListQ proto;
        MBCAF::MsgServer::VaryDepartListA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;
            uint32_t userid = proto.user_id();
            uint32_t nLastUpdate = proto.latest_update_time();
            list<uint32_t> lsChangedIds;
            Model_Department::getInstance()->getVaryDepartList(nLastUpdate, lsChangedIds);
            list<MBCAF::Proto::DepartInfo> lsDeparts;
            Model_Department::getInstance()->getDepartList(lsChangedIds, lsDeparts);

            protoA.set_user_id(userid);
            protoA.set_latest_update_time(nLastUpdate);
            for (auto it = lsDeparts.begin(); it != lsDeparts.end(); ++it)
            {
                MBCAF::Proto::DepartInfo * pDeptInfo = protoA.add_dept_list();
                pDeptInfo->set_dept_id(it->dept_id());
                pDeptInfo->set_priority(it->priority());
                pDeptInfo->set_dept_name(it->dept_name());
                pDeptInfo->set_parent_dept_id(it->parent_dept_id());
                pDeptInfo->set_dept_status(it->dept_status());
            }
            Mlog("userId=%u, last_update=%u, cnt=%u", userid, nLastUpdate, lsDeparts.size());
            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(MSMSG(BuddyOrganizationA));
            M_Only(DataSyncManager)->response(sconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //----------------------------------------------------------------
    void UserLoginValidA(ServerConnect * sconn, MdfMessage * msg)
    {
        MdfMessage * remsg = new MdfMessage;
        MBCAF::ServerBase::UserLoginValidQ proto;
        MBCAF::ServerBase::UserLoginValidA protoA;
        if (msg->toProto(&proto))
        {
            String mDomain = proto.user_name();
            String strPass = proto.password();

            protoA.set_user_name(mDomain);
            protoA.set_attach_data(proto.attach_data());

            do
            {
                ScopeLock cAutoLock(g_cLimitLock);
                list<uint32_t>& lsErrorTime = g_hmLimits[mDomain];
                uint32_t tmNow = time(NULL);

                auto itTime = lsErrorTime.begin();
                for (; itTime != lsErrorTime.end(); ++itTime)
                {
                    if (tmNow - *itTime > 30 * 60)
                    {
                        break;
                    }
                }
                if (itTime != lsErrorTime.end())
                {
                    lsErrorTime.erase(itTime, lsErrorTime.end());
                }

                if (lsErrorTime.size() > 10)
                {
                    itTime = lsErrorTime.begin();
                    if (tmNow - *itTime <= 30 * 60)
                    {
                        protoA.set_result_code(6);
                        protoA.set_result_string("用户名/密码错误次数太多");
                        remsg->setProto(&protoA);
                        remsg->setSeqIdx(msg->getSeqIdx());
                        remsg->setCommandID(SBMSG(UserLoginValidA));
                        M_Only(DataSyncManager)->response(sconn, remsg);
                        return;
                    }
                }
            } while (false);

            Mlog("%s request login.", mDomain.c_str());

            MBCAF::Proto::UserInfo info;

            if (M_Only(Model_User)->getLoginPrc()(mDomain, strPass, info))
            {
                MBCAF::Proto::UserInfo * info = protoA.mutable_user_info();
                info->set_user_id(info.user_id());
                info->set_user_gender(info.user_gender());
                info->set_department_id(info.department_id());
                info->set_user_nick_name(info.user_nick_name());
                info->set_user_domain(info.user_domain());
                info->set_avatar_url(info.avatar_url());

                info->set_email(info.email());
                info->set_user_tel(info.user_tel());
                info->set_user_real_name(info.user_real_name());
                info->set_status(0);

                info->set_sign_info(info.sign_info());

                protoA.set_result_code(0);
                protoA.set_result_string("成功");

                ScopeLock cAutoLock(g_cLimitLock);
                list<uint32_t> & lsErrorTime = g_hmLimits[mDomain];
                lsErrorTime.clear();
            }
            else
            {
                uint32_t tmCurrent = time(NULL);
                ScopeLock cAutoLock(g_cLimitLock);
                list<uint32_t>& lsErrorTime = g_hmLimits[mDomain];
                lsErrorTime.push_front(tmCurrent);

                Mlog("get result false");
                protoA.set_result_code(1);
                protoA.set_result_string("用户名/密码错误");
            }
        }
        else
        {
            protoA.set_result_code(2);
            protoA.set_result_string("服务端内部错误");
        }

        remsg->setProto(&protoA);
        remsg->setSeqIdx(msg->getSeqIdx());
        remsg->setCommandID(SBMSG(UserLoginValidA));
        M_Only(DataSyncManager)->response(sconn, remsg);
    }
    //------------------------------------------------------------------
    M_SingletonImpl(Model_User);
    //------------------------------------------------------------------
    Model_User::Model_User()
    {
        mPrc = LoginFunc;
    }
    //------------------------------------------------------------------
    Model_User::~Model_User()
    {
    }
    //------------------------------------------------------------------
    void Model_User::getChangedId(uint32_t & lasttime, list<uint32_t> & idlist)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql;
            if (lasttime == 0)
            {
                strSql = "select id, updated from MACAF_User where state != 3";
            }
            else
            {
                strSql = "select id, updated from MACAF_User where updated>=" + itostr(lasttime);
            }
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    uint32_t mID;
                    uint32_t nUpdated;
                    resSet->getValue("id", mID);
                    resSet->getValue("updated", nUpdated);
                    if (nLastTime < nUpdated)
                    {
                        nLastTime = nUpdated;
                    }
                    idlist.push_back(mID);
                }
                delete resSet;
            }
            else
            {
                Mlog(" no result set for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //------------------------------------------------------------------
    void Model_User::getUsers(const list<uint32_t> & idlist, list<MBCAF::Proto::UserInfo> & infolist)
    {
        if (idlist.empty())
        {
            Mlog("list is empty");
            return;
        }
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strClause;
            bool bFirst = true;
            for (auto it = idlist.begin(); it != idlist.end(); ++it)
            {
                if (bFirst)
                {
                    bFirst = false;
                    strClause += itostr(*it);
                }
                else
                {
                    strClause += ("," + itostr(*it));
                }
            }
            String  strSql = "select * from MACAF_User where id in (" + strClause + ")";
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    MBCAF::Proto::UserInfo info;
                    Mi32 temp;
                    resSet->getValue("id", temp);
                    info.set_user_id(temp);
                    resSet->getValue("sex", temp);
                    info.set_user_gender(temp);
                    resSet->getValue("nick", temp);
                    info.set_user_nick_name(temp);
                    resSet->getValue("domain", temp);
                    info.set_user_domain(temp);
                    resSet->getValue("name", temp);
                    info.set_user_real_name(temp);
                    resSet->getValue("phone", temp);
                    info.set_user_tel(temp);
                    resSet->getValue("email", temp);
                    info.set_email(temp);
                    resSet->getValue("avatar", temp);
                    info.set_avatar_url(temp);
                    resSet->getValue("sign_info", temp);
                    info.set_sign_info(temp);
                    resSet->getValue("departId", temp);
                    info.set_department_id(temp);
                    resSet->getValue("departId", temp);
                    info.set_department_id(temp);
                    resSet->getValue("state", temp);
                    info.set_status(temp);
                    infolist.push_back(info);
                }
                delete resSet;
            }
            else
            {
                Mlog(" no result set for sql:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //------------------------------------------------------------------
    bool Model_User::getUser(uint32_t userid, UserInfoBase &base)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select * from MACAF_User where id=" + itostr(userid);
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    resSet->getValue("id", base.mID);
                    resSet->getValue("sex", base.mSex);
                    resSet->getValue("nick", base.mNickName);
                    resSet->getValue("domain", base.mDomain);
                    resSet->getValue("name", base.mName);
                    resSet->getValue("phone", base.mTel);
                    resSet->getValue("email", base.mEmail);
                    resSet->getValue("avatar", base.mAvatarFile);
                    resSet->getValue("sign_info", base.mSignInfo);
                    resSet->getValue("departId", base.mDepartmentID);
                    resSet->getValue("state", base.mState);
                    bRet = true;
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
        return bRet;
    }
    //------------------------------------------------------------------
    bool Model_User::updateUser(UserInfoBase & base)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            uint32_t nNow = (uint32_t)time(NULL);
            String strSql = "update MACAF_User set `sex`=" + itostr(base.mSex) + ", `nick`='" + base.mNickName + "', `domain`='" + base.mDomain + "', `name`='" + base.mName + "', `phone`='" + base.mTel + "', `email`='" + base.mEmail + "', `avatar`='" + base.mAvatarFile + "', `sign_info`='" + base.mSignInfo + "', `departId`='" + itostr(base.mDepartmentID) + "', `state`=" + itostr(base.mState) + ", `updated`=" + itostr(nNow) + " where id=" + itostr(base.mID);
            bRet = dbConn->exec(strSql.c_str());
            if (!bRet)
            {
                Mlog("updateUser: update failed:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //------------------------------------------------------------------
    bool Model_User::insertUser(UserInfoBase & base)
    {
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "insert into MACAF_User(`id`,`sex`,`nick`,`domain`,`name`,`phone`,`email`,`avatar`,`sign_info`,`departId`,`state`,`created`,`updated`) values(?,?,?,?,?,?,?,?,?,?,?,?)";
            PrepareExec* stmt = new PrepareExec();
            if (stmt->prepare(dbConn->getConnect(), strSql))
            {
                uint32_t nNow = (uint32_t)time(NULL);
                uint32_t index = 0;
                uint32_t nGender = base.mSex;
                uint32_t mState = base.mState;
                stmt->setParam(index++, base.mID);
                stmt->setParam(index++, nGender);
                stmt->setParam(index++, base.mNickName);
                stmt->setParam(index++, base.mDomain);
                stmt->setParam(index++, base.mName);
                stmt->setParam(index++, base.mTel);
                stmt->setParam(index++, base.mEmail);
                stmt->setParam(index++, base.mAvatarFile);

                stmt->setParam(index++, base.mSignInfo);
                stmt->setParam(index++, base.mDepartmentID);
                stmt->setParam(index++, mState);
                stmt->setParam(index++, nNow);
                stmt->setParam(index++, nNow);
                bRet = stmt->exec();

                if (!bRet)
                {
                    Mlog("insert user failed: %s", strSql.c_str());
                }
            }
            delete stmt;
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
        return bRet;
    }
    //------------------------------------------------------------------
    void Model_User::clearUserCounter(uint32_t userid, uint32_t sessionid, MBCAF::Proto::SessionType type)
    {
        if (MBCAF::Proto::SessionType_IsValid(type))
        {
            MembaseManager * memdbMag = M_Only(MembaseManager);
            MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
            if (memdbConn)
            {
                if (type == MBCAF::Proto::ST_Single)
                {
                    int nRet = memdbConn->HDEL("unread_" + itostr(userid), itostr(sessionid));
                    if (!nRet)
                    {
                        Mlog("HDEL failed %d->%d", sessionid, userid);
                    }
                }
                else if (type == MBCAF::Proto::ST_Group)
                {
                    String strGroupKey = itostr(sessionid) + ChatGroupMsg;
                    map<String, String> mapGroupCount;
                    bool bRet = memdbConn->HGETALL(strGroupKey, mapGroupCount);
                    if (bRet)
                    {
                        String strUserKey = itostr(userid) + "_" + itostr(sessionid) + ChatUserGroupMsg;
                        String strReply = memdbConn->HMSET(strUserKey, mapGroupCount);
                        if (strReply.empty()) 
                        {
                            Mlog("HMSET %s failed !", strUserKey.c_str());
                        }
                    }
                    else
                    {
                        Mlog("hgetall %s failed!", strGroupKey.c_str());
                    }

                }
                memdbMag->freeTempConnect(memdbConn);
            }
            else
            {
                Mlog("no cache connection for unread");
            }
        }
        else
        {
            Mlog("invalid sessionType. userId=%u, fromId=%u, sessionType=%u", userid, sessionid, type);
        }
    }
    //------------------------------------------------------------------
    void Model_User::setCallReport(uint32_t userid, uint32_t sessionid, MBCAF::Proto::ClientType nClientType)
    {
        if (MBCAF::Proto::ClientType_IsValid(nClientType))
        {
            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
            if (dbConn)
            {
                String strSql = "insert into MBCAF_CallLog(`userId`, `peerId`, `clientType`,`created`,`updated`) values(?,?,?,?,?)";
                PrepareExec* stmt = new PrepareExec();
                if (stmt->prepare(dbConn->getConnect(), strSql))
                {
                    uint32_t nNow = (uint32_t)time(NULL);
                    uint32_t index = 0;
                    uint32_t nClient = (uint32_t)nClientType;
                    stmt->setParam(index++, userid);
                    stmt->setParam(index++, sessionid);
                    stmt->setParam(index++, nClient);
                    stmt->setParam(index++, nNow);
                    stmt->setParam(index++, nNow);
                    bool bRet = stmt->exec();

                    if (!bRet)
                    {
                        Mlog("insert report failed: %s", strSql.c_str());
                    }
                }
                delete stmt;
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsmaster");
            }

        }
        else
        {
            Mlog("invalid clienttype. userId=%u, peerId=%u, clientType=%u", userid, sessionid, nClientType);
        }
    }
    //------------------------------------------------------------------
    bool Model_User::updateUserSignInfo(uint32_t userid, const String & str)
    {
        if (str.length() > 128)
        {
            Mlog("updateUserSignInfo: sign_info.length()>128.\n");
            return false;
        }
        bool rv = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            uint32_t now = (uint32_t)time(NULL);
            String str_sql = "update MACAF_User set `sign_info`='" + str + "', `updated`=" + itostr(now) + " where id=" + itostr(userid);
            rv = dbConn->exec(str_sql.c_str());
            if (!rv)
            {
                Mlog("updateUserSignInfo: update failed:%s", str_sql.c_str());
            }
            else
            {
                M_Only(DataSyncManager)->setUserUpdate(now);
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("updateUserSignInfo: no db connection for gsgsmaster");
        }
        return rv;
    }
    //------------------------------------------------------------------
    bool Model_User::getUserSingInfo(uint32_t userid, String* str)
    {
        bool rv = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String str_sql = "select sign_info from MACAF_User where id=" + itostr(userid);
            DatabaseResult* result_set = dbConn->execQuery(str_sql.c_str());
            if (result_set)
            {
                if (result_set->nextRow())
                {
                    *str = result_set->getValue("sign_info");
                    rv = true;
                }
                delete result_set;
            }
            else
            {
                Mlog("no result set for sql:%s", str_sql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
        return rv;
    }
    //------------------------------------------------------------------
    bool Model_User::updatePushShield(uint32_t userid, uint32_t state)
    {
        bool rv = false;

        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            uint32_t now = (uint32_t)time(NULL);
            String str_sql = "update MACAF_User set `push_shield_status`=" + itostr(state) + ", `updated`=" + itostr(now) + " where id=" + itostr(userid);
            rv = dbConn->exec(str_sql.c_str());
            if (!rv)
            {
                Mlog("updatePushShield: update failed:%s", str_sql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("updatePushShield: no db connection for gsgsmaster");
        }

        return rv;
    }
    //------------------------------------------------------------------
    bool Model_User::getPushShield(uint32_t userid, uint32_t* state)
    {
        bool rv = false;

        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String str_sql = "select push_shield_status from MACAF_User where id=" + itostr(userid);
            DatabaseResult * result_set = dbConn->execQuery(str_sql.c_str());
            if (result_set)
            {
                if (result_set->nextRow())
                {
                    result_set->getValue("push_shield_status", *state);
                    rv = true;
                }
                delete result_set;
            }
            else
            {
                Mlog("getPushShield: no result set for sql:%s", str_sql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("getPushShield: no db connection for gsgsslave");
        }

        return rv;
    }
    //------------------------------------------------------------------
    LoginPrc * Model_User::getLoginPrc() const
    {
        return mPrc;
    }
    //------------------------------------------------------------------
}