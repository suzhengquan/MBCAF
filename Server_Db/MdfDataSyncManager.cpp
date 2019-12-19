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

#include "MdfDataSyncManager.h"
#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfThreadManager.h"
#include "MdfModel_User.h"
#include "MdfModel_Group.h"
#include "MdfModel_Session.h"
#include "MBCAF.Proto.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //------------------------------------------------------------------
    typedef struct ConnectMessage_t
    {
        ServerConnect * mConnect;
        Message * mMessage;
    } ConnectMessage;
    //------------------------------------------------------------------
    M_SingletonImpl(DataSyncManager);
    //------------------------------------------------------------------
    DataSyncManager::DataSyncManager() :
        mLastGroup(time(NULL)),
        mMsgThread(0),
        mSyncThread(0)
    {
        mGroupCondition = new ThreadCondition(&mGroupMutex);
        mSyncGroup = false;

        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_UserLoginValidQ), Mdf::UserLoginValidA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_PushShieldSQ), Mdf::PushShieldSA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_PushShieldQ), Mdf::PushShieldA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BubbyRecentSessionQ), Mdf::RecentSessionA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BubbyRemoveSessionQ), Mdf::RemoveSessionA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BubbyObjectInfoQ), Mdf::UserInfoListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BuddyObjectListQ), Mdf::VaryUserInfoListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BuddyOrganizationQ), Mdf::VaryDepartListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_BuddyChangeSignatureQ), Mdf::SignInfoSA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_Data), Mdf::SendMessage));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_MsgListQ), Mdf::MessageInfoListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_UnReadCountQ), Mdf::MessageUnreadCntA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_ReadACK), Mdf::MessageReadA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_MsgQ), Mdf::MessageInfoByIdA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_RecentMsgQ), Mdf::MessageRecentA));

        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_TrayMsgQ), Mdf::TrayMsgA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_SwitchTrayMsgQ), Mdf::SwitchTrayMsgA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_GroupShieldSQ), Mdf::GroupShieldSA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_ServerBase | SBID_GroupShieldQ), Mdf::GroupShieldA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_GroupListQ), Mdf::GroupListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_GroupInfoQ), Mdf::GroupInfoListA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_CreateGroupQ), Mdf::GroupCreateA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_MsgServer | MSID_GroupMemberSQ), Mdf::GroupMemberSA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_HubServer | HSID_OfflineFileQ), Mdf::OfflineFileExistA));
        mMsgPrcList.insert(make_pair(Mui32(MTID_HubServer | HSID_AddOfflineFileQ), Mdf::addOfflineFile));
        mMsgPrcList.insert(make_pair(Mui32(MTID_HubServer | HSID_DeleteOfflineFileQ), Mdf::deleteOfflineFile));

        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            String strTotalUpdate = memdbConn->GET("total_user_updated");

            String strLastUpdateGroup = memdbConn->GET("last_update_group");
            memdbMag->freeTempConnect(memdbConn);
            if (strTotalUpdate != "")
            {
                mLastUser = strtoi(strTotalUpdate);
            }
            else
            {
                setUserUpdate(time(NULL));
            }
            if (strLastUpdateGroup.empty())
            {
                mLastGroup = strtoi(strLastUpdateGroup);
            }
            else
            {
                updateGroup(time(NULL));
            }
        }
        else
        {
            Mlog("no cache connection to get total_user_updated");
        }

        mSyncThread = M_Only(ThreadManager)->create(1);
        mSyncThread->add(this);

        setEnable(true);
    }
    //------------------------------------------------------------------
    DataSyncManager::~DataSyncManager()
    {
        if (mGroupCondition)
        {
            mGroupCondition->signal();
            while (mSyncGroup)
            {
                usleep(500);
            }

            delete mGroupCondition;
            mGroupCondition = 0;
        }
        if (mMsgThread)
        {
            M_Only(ThreadManager)->destroy(mMsgThread);
            mMsgThread = 0;
        }
        if (mSyncThread)
        {
            M_Only(ThreadManager)->destroy(mSyncThread);
            mSyncThread = 0;
        }
        setEnable(false);
    }
    //-----------------------------------------------------------------------
    void DataSyncManager::run()
    {
        mSyncGroup = true;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        map<Mui32, Mui32> mapChangedGroup;
        do 
        {
            mapChangedGroup.clear();
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
            if (dbConn)
            {
                Mui32 ttmp;
                {
                    ScopeLock scope(mUpdateMute);
                    Mui32 ttmp = mLastGroup;
                }
                String strSql = "select id, lastChated from MACAF_Group where state=0 and lastChated >=" + itostr(ttmp);
                DatabaseResult * dbRes = dbConn->execQuery(strSql.c_str());
                if (dbRes)
                {
                    while (dbRes->nextRow())
                    {
                        Mui32 nGroupId;
                        Mui32 nLastChat
                        dbRes->getValue("id", nGroupId);
                        dbRes->getValue("lastChated", nLastChat);
                        if (nLastChat != 0)
                        {
                            mapChangedGroup[nGroupId] = nLastChat;
                        }
                    }
                    delete dbRes;
                }
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsslave");
            }
            updateGroup(time(NULL));
            for (auto it = mapChangedGroup.begin(); it != mapChangedGroup.end(); ++it)
            {
                Mui32 nGroupId = it->first;
                list<Mui32> lsUsers;
                Mui32 nUpdate = it->second;
                M_Only(Model_Group)->getGroupUser(nGroupId, lsUsers);
                for (auto it1 = lsUsers.begin(); it1 != lsUsers.end(); ++it1)
                {
                    Mui32 userid = *it1;
                    Mui32 nSessionId = INVALID_VALUE;
                    nSessionId = M_Only(Model_Session)->getSessionId(userid, nGroupId, MBCAF::Proto::ST_Group, true);
                    if (nSessionId != INVALID_VALUE)
                    {
                        M_Only(Model_Session)->updateSession(nSessionId, nUpdate);
                    }
                    else
                    {
                        M_Only(Model_Session)->addSession(userid, nGroupId, MBCAF::Proto::ST_Group);
                    }
                }
            }
        } while (!(mGroupCondition->wait(5 * 1000)));
        mSyncGroup = false;
    }
    //------------------------------------------------------------------
    Mui32 DataSyncManager::getUserUpdate()
    {
        ScopeLock scope(mUpdateMute);
        return mLastUser;
    }
    //------------------------------------------------------------------
    void DataSyncManager::setUserUpdate(Mui32 time)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            mUpdateMute.acquire();
            mLastUser = time;
            mUpdateMute.release();

            String strUpdated = itostr(time);
            memdbConn->SET("total_user_update", strUpdated);
            memdbMag->freeTempConnect(memdbConn);
        }
    }
    //------------------------------------------------------------------
    MessagePrc DataSyncManager::getPrc(Mui32 id)
    {
        MsgPrcList::iterator it = mMsgPrcList.find(id);
        if (it != mMsgPrcList.end())
        {
            return it->second;
        }
        else
        {
            return NULL;
        }
    }
    //------------------------------------------------------------------
    void DataSyncManager::setupMsgThread(MCount cnt)
    {
        mMsgThread = M_Only(ThreadManager)->create(cnt);
    }
    //------------------------------------------------------------------
    Thread * DataSyncManager::getMsgThread() const
    {
        return mMsgThread;
    }
    //------------------------------------------------------------------
    void DataSyncManager::response(ServerConnect * connect, Message * msg)
    {
        ConnectMessage * remsg = new ConnectMessage;
        remsg->mConnect = connect;
        remsg->mMessage = msg;

        mResponseMutex.acquire();
        mResponseList.push_back(remsg);
        mResponseMutex.release();
    }
    //------------------------------------------------------------------
    void DataSyncManager::onTimer(TimeDurMS tick)
    {
        mResponseMutex.acquire();
        while (!mResponseList.empty())
        {
            ConnectMessage * msg = mResponseList.front();
            mResponseList.pop_front();
            mResponseMutex.release();

            ServerConnect * pConn = msg->mConnect;
            if (pConn)
            {
                if (msg->mMessage)
                {
                    pConn->send(msg->mMessage);
                }
                else
                {
                    Mlog("close connection uuid=%d by parse pdu error\b", msg->mConnect);
                    pConn->stop();
                }
            }

            if (msg->mMessage)
                delete msg->mMessage;
            delete msg;

            mResponseMutex.acquire();
        }

        mResponseMutex.release();
    }
    //------------------------------------------------------------------
    void DataSyncManager::updateGroup(Mui32 time)
    {
        MembaseManager * memdbMag = M_Only(MembaseManager);
        MembaseConnect * memdbConn = memdbMag->getTempConnect("unread");
        if (memdbConn)
        {
            mUpdateMute.acquire();
            mLastGroup = time;
            mUpdateMute.release();

            String strUpdated = itostr(time);
            memdbConn->SET("last_update_group", strUpdated);
            memdbMag->freeTempConnect(memdbConn);
        }
    }
    //------------------------------------------------------------------
}