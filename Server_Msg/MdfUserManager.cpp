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

#include "MdfUserManager.h"
#include "MdfServerConnect.h"
#include "MdfRouteClientConnect.h"
#include "MBCAF.ServerBase.pb.h"

using namespace ::MBCAF::Proto;

namespace Mdf
{
    //--------------------------------------------------------------------------
    User::User(const String & name)
    {
        mID = 0;
        mLoginID = name;
        mValid = false;
        mPCState = MBCAF::Proto::OST_Offline;
    }
    //--------------------------------------------------------------------------
    User::~User()
    {
    }
    //--------------------------------------------------------------------------
    ServerConnect * User::getMsgConnect(ConnectID handle)
    {
        ServerConnect * pMsgConn = NULL;
        map<Mui32, ServerConnect *>::iterator it = mConnectList.find(handle);
        if (it != mConnectList.end())
        {
            pMsgConn = it->second;
        }
        return pMsgConn;
    }
    //--------------------------------------------------------------------------
    void User::addMsgConnect(ConnectID handle, ServerConnect * conn)
    {
        mConnectList[handle] = conn;
        mUnvalidConnectList.erase(conn);
    }
    //--------------------------------------------------------------------------
    void User::removeMsgConnect(ConnectID handle)
    { 
        mConnectList.erase(handle); 
    }
    //--------------------------------------------------------------------------
    ServerConnect * User::getUnvalidMsgConnect(ConnectID handle)
    {
        UnvalidList::iterator it, itend = mUnvalidConnectList.end();
        for (it = mUnvalidConnectList.begin(); it != itend; it++)
        {
            ServerConnect * conn = *it;
            if (conn->getID() == handle)
            {
                return conn;
            }
        }

        return 0;
    }
    //--------------------------------------------------------------------------
    void User::addUnvalidMsgConnect(ServerConnect * conn) 
    { 
        mUnvalidConnectList.insert(conn); 
    }
    //--------------------------------------------------------------------------
    void User::removeUnvalidMsgConnect(ServerConnect * conn) 
    { 
        mUnvalidConnectList.erase(conn); 
    }
    //--------------------------------------------------------------------------
    const User::ConnectList & User::GetMsgConnMap() const
    { 
        return mConnectList; 
    }
    //--------------------------------------------------------------------------
    void User::broadcast(Message * msg, ServerConnect * qconn)
    {
        map<Mui32, ServerConnect*>::iterator it, itend = mConnectList.end();
        for (it = mConnectList.begin(); it != itend; ++it)
        {
            ServerConnect * conn = it->second;
            if (conn != qconn)
            {
                conn->send(msg);
            }
        }
    }
    //--------------------------------------------------------------------------
    void User::broadcastToPC(Message *msg, ServerConnect* qconn)
    {
        map<Mui32, ServerConnect*>::iterator it, itend = mConnectList.end();
        for (it = mConnectList.begin(); it != itend; ++it)
        {
            ServerConnect* conn = it->second;
            if (conn != qconn && M_PCLoginCheck(conn->getClientType()))
            {
                conn->send(msg);
            }
        }
    }
    //--------------------------------------------------------------------------
    void User::broadcastToMobile(Message * msg, ServerConnect * qconn)
    {
        map<Mui32, ServerConnect*>::iterator it, itend = mConnectList.end();
        for (it = mConnectList.begin(); it != itend; it++)
        {
            ServerConnect* conn = it->second;
            if (conn != qconn && M_MobileLoginCheck(conn->getClientType()))
            {
                conn->send(msg);
            }
        }
    }
    //--------------------------------------------------------------------------
    void User::broadcastFromClient(Message * msg, Mui32 msg_id, ServerConnect * qconn, Mui32 from_id)
    {
        map<Mui32, ServerConnect *>::iterator it, itend = mConnectList.end();
        for (it = mConnectList.begin(); it != itend; ++it)
        {
            ServerConnect * conn = it->second;
            if (conn != qconn)
            {
                conn->send(msg);
                conn->addSend(msg_id, from_id);
            }
        }
    }
    //--------------------------------------------------------------------------
    void User::broadcast(void * data, Mui32 len, ServerConnect * qconn)
    {
        if (!data)
            return;
        map<Mui32, ServerConnect*>::iterator it, itend = mConnectList.end();
        for (it = mConnectList.begin(); it != itend; ++it)
        {
            ServerConnect * conn = it->second;
            if (conn != qconn)
            {
                conn->send(data, len);
            }
        }
    }
    //--------------------------------------------------------------------------
    void User::kickUser(ServerConnect * conn, Mui32 reason)
    {
        map<Mui32, ServerConnect*>::iterator it = mConnectList.find(conn->getID());
        if (it != mConnectList.end())
        {
            ServerConnect * conn = it->second;
            if (conn)
            {
                Mlog("kick service user, user_id=%u.", mID);
                MBCAF::ServerBase::KickConnect proto;
                proto.set_user_id(mID);
                proto.set_kick_reason((::MBCAF::Proto::KickReasonType)reason);
                Message remsg;
                remsg.setProto(&proto);
                remsg.setCommandID(SBMSG(KickUser));
                
                conn->send(&remsg);
                conn->setKickOff(true);
            }
        }
    }
    //--------------------------------------------------------------------------
    bool User::kickSameClientOut(Mui32 ctype, Mui32 reason, ServerConnect* pFromConn)
    {
        for(map<Mui32, ServerConnect*>::iterator it = mConnectList.begin(); it != mConnectList.end(); it++)
        {
            ServerConnect * pMsgConn = it->second;

            if ((((pMsgConn->getClientType() ^ ctype) >> 4) == 0) && (pMsgConn != pFromConn))
            {
                kickUser(pMsgConn, reason);
                break;
            }
        }
        return true;
    }
    //--------------------------------------------------------------------------
    Mui32 User::getClientTypeMark()
    {
        Mui32 client_type_flag = 0x00;
        map<Mui32, ServerConnect*>::iterator it = mConnectList.begin();
        for (; it != mConnectList.end(); it++)
        {
            ServerConnect* conn = it->second;
            Mui32 ctype = conn->getClientType();
            if (M_PCLoginCheck(ctype))
            {
                client_type_flag |= CLIENT_TYPE_FLAG_PC;
            }
            else if (M_MobileLoginCheck(ctype))
            {
                client_type_flag |= CLIENT_TYPE_FLAG_MOBILE;
            }
        }
        return client_type_flag;
    }
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    // UserManager
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    M_SingletonImpl(UserManager);
    //--------------------------------------------------------------------------
    UserManager::UserManager()
    {
    }
    //--------------------------------------------------------------------------
    UserManager::~UserManager()
    {
        destroyAll();
    }
    //--------------------------------------------------------------------------
    User * UserManager::getUser(const String & name)
    {
        UserNameList::iterator it = mUserNameList.find(name);
        if (it != mUserNameList.end())
        {
            return it->second;
        }
        return 0;
    }
    //--------------------------------------------------------------------------
    User * UserManager::getUser(Mui32 id)
    {
        UserList::iterator it = mUserList.find(id);
        if (it != mUserList.end())
        {
            return it->second;
        }
        return 0;
    }
    //--------------------------------------------------------------------------
    ServerConnect * UserManager::getMsgConnect(Mui32 id, ConnectID cid)
    {
        ServerConnect * pMsgConn = NULL;
        User * userobj = getUser(id);
        if (userobj)
        {
            pMsgConn = userobj->getMsgConnect(cid);
        }
        return pMsgConn;
    }
    //--------------------------------------------------------------------------
    bool UserManager::addUser(const String & name, User * tempusr)
    {
        bool bRet = false;
        if (getUser(name) == NULL)
        {
            mUserNameList[name] = tempusr;
            bRet = true;
        }
        return bRet;
    }
    //--------------------------------------------------------------------------
    void UserManager::removeUser(const String & name)
    {
        mUserNameList.erase(name);
    }
    //--------------------------------------------------------------------------
    bool UserManager::addUser(Mui32 id, User *tempusr)
    {
        bool bRet = false;
        if (getUser(id) == NULL)
        {
            mUserList[id] = tempusr;
            bRet = true;
        }
        return bRet;
    }
    //--------------------------------------------------------------------------
    void UserManager::removeUser(Mui32 id)
    {
        mUserList.erase(id);
    }
    //--------------------------------------------------------------------------
    void UserManager::destroyUser(User * obj)
    {
        if (obj != NULL)
        {
            mUserList.erase(obj->getID();
            mUserNameList.erase(obj->getLoginID());
            delete obj;
            obj = NULL;
        }
    }
    //--------------------------------------------------------------------------
    void UserManager::destroyAll()
    {
        UserNameList::iterator it, itend = mUserNameList.end();
        for (it = mUserNameList.begin(); it != itend; ++it)
        {
            User* tempusr = it->second;
            if (tempusr != NULL)
            {
                delete tempusr;
                tempusr = NULL;
            }
        }
        mUserNameList.clear();
        mUserList.clear();
    }
    //--------------------------------------------------------------------------
    void UserManager::getUserInfoList(list<UserState> & infolist)
    {
        UserState state;
        User * userobj = NULL;
        for (UserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it)
        {
            userobj = (User*)it->second;
            if (userobj->isValid())
            {
                map<Mui32, ServerConnect*> & ConnMap = userobj->GetMsgConnMap();
                for (map<Mui32, ServerConnect*>::iterator it2 = ConnMap.begin(); it2 != ConnMap.end(); ++it2)
                {
                    ServerConnect* conn = it2->second;
                    if (conn->isOpen())
                    {
                        state.mUserID = userobj->getID();
                        state.mClientType = conn->getClientType();
                        state.mState = conn->getOnlineState();
                        infolist.push_back(state);
                    }
                }
            }
        }
    }
    //--------------------------------------------------------------------------
    void UserManager::getUserConnectList(vector<Mui32> & idlist, vector<Mui32> & connlist, Mui32 & totalconn)
    {
        totalconn = 0;
        User * userobj = NULL;
        for (UserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it)
        {
            userobj = (User*)it->second;
            if (userobj->isValid())
            {
                Mui32 cnt = 0;
                User::ConnectList::iterator it, itend = userobj->GetMsgConnMap().end();
                for(it = userobj->GetMsgConnMap().begin(); it != itend; ++it)
                {
                    ServerConnect * conn = it->second;
                    if (conn->isOpen())
                    {
                        cnt++;
                    }
                }
  
                idlist.push_back(userobj->getID());
                connlist.push_back(cnt);
                totalconn += cnt;
            }
        }
    }
    //--------------------------------------------------------------------------
    void UserManager::broadcast(Message * msg, Mui32 client_type_flag)
    {
        User * userobj = NULL;
        UserList::iterator it, itend = mUserList.end();
        for (it = mUserList.begin(); it != itend; ++it)
        {
            userobj = (User*)it->second;
            if (userobj->isValid())
            {
                switch (client_type_flag)
                {
                case CLIENT_TYPE_FLAG_PC:
                    userobj->broadcastToPC(msg);
                    break;
                case CLIENT_TYPE_FLAG_MOBILE:
                    userobj->broadcastToMobile(msg);
                    break;
                case CLIENT_TYPE_FLAG_BOTH:
                    userobj->broadcast(msg);
                    break;
                default:
                    break;
                }
            }
        }
    }
    //--------------------------------------------------------------------------
}