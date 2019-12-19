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

#ifndef _MDF_USERMANAGER_H_
#define _MDF_USERMANAGER_H_

#include "MdfPreInclude.h"

namespace Mdf
{
    #define MAX_ONLINE_FRIEND_CNT 100

    class ServerConnect;

    typedef struct
    {
        Mui32 mUserID;
        Mui32 mState;
        Mui32 mClientType;
    } UserState;

    /**
    @version 0.9.1
    */
    class User
    {
    public:
        typedef map<ConnectID, ServerConnect *> ConnectList;
        typedef set<ServerConnect *> UnvalidList;
    public:
        User(const String & name);
        ~User();

        /**
        @version 0.9.1
        */
        void setID(Mui32 id) { mID = id; }

        /**
        @version 0.9.1
        */
        Mui32 getID() const { return mID; }

        /**
        @version 0.9.1
        */
        const String & getLoginID() const { return mLoginID; }

        /**
        @version 0.9.1
        */
        void setNick(const String & nick) { mNick = nick; }

        /**
        @version 0.9.1
        */
        const String & getNick() const { return mNick; }

        /**
        @version 0.9.1
        */
        void setValid(bool set) { mValid = set; }

        /**
        @version 0.9.1
        */
        bool isValid() const { return mValid; }

        /**
        @version 0.9.1
        */
        void setPCState(Mui32 state) { mPCState = state; }

        /**
        @version 0.9.1
        */
        Mui32 getPCState() const { return mPCState; }

        /**
        @version 0.9.1
        */
        void addMsgConnect(ConnectID id, ServerConnect * conn);

        /**
        @version 0.9.1
        */
        void removeMsgConnect(ConnectID id);

        /**
        @version 0.9.1
        */
        ServerConnect * getMsgConnect(ConnectID handle);

        /**
        @version 0.9.1
        */
        void addUnvalidMsgConnect(ServerConnect * conn);

        /**
        @version 0.9.1
        */
        void removeUnvalidMsgConnect(ServerConnect * conn);

        /**
        @version 0.9.1
        */
        ServerConnect * getUnvalidMsgConnect(ConnectID handle);

        /**
        @version 0.9.1
        */
        const ConnectList & GetMsgConnMap() const;

        /**
        @version 0.9.1
        */
        void broadcast(void * data, Mui32 size, ServerConnect * qconn = NULL);

        /**
        @version 0.9.1
        */
        void broadcast(Message * msg, ServerConnect * qconn = NULL);

        /**
        @version 0.9.1
        */
        void broadcastToPC(Message * msg, ServerConnect * qconn = NULL);

        /**
        @version 0.9.1
        */
        void broadcastToMobile(Message * msg, ServerConnect * qconn = NULL);

        /**
        @version 0.9.1
        */
        void broadcastFromClient(Message * msg, Mui32 msg_id, ServerConnect * qconn = NULL, Mui32 from_id = 0);

        /**
        @version 0.9.1
        */
        void kickUser(ServerConnect * conn, Mui32 reason);

        /**
        @version 0.9.1
        */
        bool kickSameClientOut(Mui32 ctype, Mui32 reason, ServerConnect * qconn = NULL);

        /**
        @version 0.9.1
        */
        Mui32 getClientTypeMark();
    private:
        Mui32 mID;
        String mLoginID;
        String mNick;
        Mui32 mPCState;
        ConnectList mConnectList;
        UnvalidList mUnvalidConnectList;
        bool mValid;
    };

    typedef map<Mui32, User *> UserList;
    typedef map<String, User *> UserNameList;

    /**
    @version 0.9.1
    */
    class UserManager
    {
    public:
        UserManager();
        ~UserManager();

        /**
        @version 0.9.1
        */
        User * getUser(Mui32 id);

        /**
        @version 0.9.1
        */
        User * getUser(const String & name);

        /**
        @version 0.9.1
        */
        ServerConnect * getMsgConnect(Mui32 id, ConnectID cid);

        /**
        @version 0.9.1
        */
        bool addUser(const String & name, User * obj);

        /**
        @version 0.9.1
        */
        bool addUser(Mui32 id, User * obj);

        /**
        @version 0.9.1
        */
        void removeUser(const String & name);

        /**
        @version 0.9.1
        */
        void removeUser(Mui32 id);

        /**
        @version 0.9.1
        */
        void destroyUser(User * obj);

        /**
        @version 0.9.1
        */
        void destroyAll();

        /**
        @version 0.9.1
        */
        void getUserInfoList(list<UserState> & infolist);

        /**
        @version 0.9.1
        */
        void getUserConnectList(vector<Mui32> & idlist, vector<Mui32> & connlist, Mui32 & totalconn);

        /**
        @version 0.9.1
        */
        void broadcast(Message * pdu, Mui32 ctype);
    private:
        UserList mUserList;
        UserNameList mUserNameList;
    };
    M_SingletonDef(UserManager);
}
#endif
