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

#ifndef _MDF_MEMBASEMANAGER_H_
#define _MDF_MEMBASEMANAGER_H_

#include "MdfPreInclude.h"
#include "MdfThreadPool.h"

#define ChatGroupMsg            "chat_groupmsg"
#define ChatUserGroupMsg        "chat_usergroupmsg"
#define ChatMsgCount            "count"

struct redisContext;

namespace Mdf
{
    class MembaseInstance;

    /**
    @version 0.9.1
    */
    class MembaseConnect
    {
    public:
        MembaseConnect(MembaseInstance * instance);
        virtual ~MembaseConnect();
        const String & getName();

        int connect();

        String GET(const String & key);
        String SET(const String & key, const String & value);
        String SETEX(const String & key, int timeout, const String & value);
        bool EXISTS(const String & key);

        long HDEL(const String & key, const String & field);
        String HGET(const String & key, const String & field);
        bool HGETALL(const String & key, map<String, String> & out);
        long HSET(const String & key, const String & field, const String & value);

        bool MGET(const vector<String> & keys, map<String, String> & out);

        long HINCRBY(const String & key, const String & field, long value);
        long INCRBY(const String & key, long value);
        String HMSET(const String & key, map<String, String> & hash);
        bool HMGET(const String & key, const list<String> & fields, list<String> & out);

        long INCR(const String & key);
        long DECR(const String & key);
        long LPUSH(const String & key, const String & value);
        long RPUSH(const String & key, const String & value);
        long LLEN(const String & key);
        bool LRANGE(const String & key, long start, long end, list<String> & out);
    private:
        MembaseInstance * mInstance;
        redisContext * mContext;
        uint64_t mLastConnect;
        bool mInit;
    };

    /**
    @version 0.9.1
    */
    class MembaseInstance
    {
    public:
        MembaseInstance(const String & name, const String & ip, Mui32 port, Mui32 dbno, Mui32 connectMax);
        virtual ~MembaseInstance();

        /**
        @version 0.9.1
        */
        int init();

        /**
        @version 0.9.1
        */
        const String & getName() const { return mName; }

        /**
        @version 0.9.1
        */
        const String & getIP() cosnt { return mIP; }

        /**
        @version 0.9.1
        */
        int getPort() const { return mPort; }

        /**
        @version 0.9.1
        */
        int getNum() const { return mDbNo; }

        /**
        @version 0.9.1
        */
        MembaseConnect * getTempConnect() const;

        /**
        @version 0.9.1
        */
        void freeTempConnect(MembaseConnect * obj);
    private:
        String mName;
        String mIP;
        Mui32 mPort;
        Mui32 mDbNo;
        Mui32 mConnectCount;
        Mui32 mConnectMax;
        list<MembaseConnect *> mFreeList;
        ThreadCondition mListCondition;
    };

    /**
    @version 0.9.1
    */
    class MembaseManager
    {
    public:
        virtual ~MembaseManager();

        /**
        @version 0.9.1
        */
        MembaseConnect * getTempConnect(const String & name);

        /**
        @version 0.9.1
        */
        void freeTempConnect(MembaseConnect * obj);
    private:
        MembaseManager();
    private:
        map<String, MembaseInstance *> mInstanceList;
    };
    M_SingletonDef(MembaseManager);
}
#endif
