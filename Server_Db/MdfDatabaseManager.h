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

#ifndef _MDF_DATABASE_MANAGER_H_
#define _MDF_DATABASE_MANAGER_H_

#include "MdfPreInclude.h"
#include "MdfThreadPool.h"

namespace Mdf
{
    class DatabaseInstance;

    /**
    @version 0.9.1
    */
    class DatabaseResult
    {
    public:
        DatabaseResult();
        virtual ~DatabaseResult();

        virtual bool nextRow() = 0;
        virtual void getValue(const String & key, Mi32 & out) = 0;
        virtual void getValue(const String & key, String & out) = 0;
        int getIndex(const String & key);
    private:
        map<String, Mui32> mFieldList;
    };

    /**
    @version 0.9.1
    */
    class PrepareExec
    {
    public:
        PrepareExec();
        virtual ~PrepareExec();

        virtual bool exec() = 0;
        virtual bool prepare(DatabaseConnect * connect, const String & sql, Mui32 paramcnt) = 0;

        virtual void setParam(Mui32 index, Mi32 & value) = 0;
        virtual void setParam(Mui32 index, Mui32 & value) = 0;
        virtual void setParam(Mui32 index, const String & value) = 0;

        virtual Mui32 getInsertId(const String & tblname, const String & colname) = 0;
    private:
        Mui32 mParamCount;
    };

    /**
    @version 0.9.1
    */
    class DatabaseConnect
    {
    public:
        DatabaseConnect(DatabaseInstance * ins);
        virtual ~DatabaseConnect();

        /**
        @version 0.9.1
        */
        const String & getName() const;

        /**
        @version 0.9.1
        */
        virtual int connect() = 0;

        /**
        @version 0.9.1
        */
        virtual bool exec(const String & query) = 0;

        /**
        @version 0.9.1
        */
        virtual DatabaseResult * execQuery(const String & query) = 0;

        /**
        @version 0.9.1
        */
        virtual void escape(const String & in, String & out) = 0;

        /**
        @version 0.9.1
        */
        virtual Mui32 getInsertId(const String & tblname, const String & colname) = 0;

        /**
        @version 0.9.1
        */
        virtual bool beginTransaction() = 0;

        /**
        @version 0.9.1
        */
        virtual bool endTransaction() = 0;

        /**
        @version 0.9.1
        */
        virtual bool rollbackTransaction() = 0;

        /**
        @version 0.9.1
        */
        virtual PrepareExec * createPrepare() const = 0;
    private:
        DatabaseInstance * mInstance;
    };

    class DatabaseInstance
    {
    public:
        DatabaseInstance(const String & name, const String & serverIp, Mui16 serverPort,
            const String & username, const String & pw, const String & dbname, Mui32 connectMax);
        virtual ~DatabaseInstance();

        int init();

        DatabaseConnect * getTempConnect();

        void freeTempConnect(DatabaseConnect * pConn);

        const String & getName() const { return mName; }
        const String & getIP() const { return mIP; }
        Mui16 getPort() const { return mPort; }
        const String & getUserName() const { return mUserName; }
        const String & getPasswrod()const { return mPassword; }
        const String & getDbName()const { return mDbName; }
    private:
        String mName;
        String mIP;
        Mui16 mPort;
        String mUserName;
        String mPassword;
        String mDbName;
        Mui32 mConnectCount;
        Mui32 mConnectMax;
        list<DatabaseConnect *> mFreeList;
        ThreadCondition mListCondition;
    };

    /**
    @version 0.9.1
    */
    class DatabaseManager
    {
    public:
        virtual ~DatabaseManager();

        /**
        @version 0.9.1
        */
        DatabaseConnect * getTempConnect(const String & name);

        /**
        @version 0.9.1
        */
        void freeTempConnect(DatabaseConnect * pConn);
    private:
        DatabaseManager();
    private:
        map<String, DatabaseInstance *> mInstanceList;
    };
    M_SingletonDef(DatabaseManager);
}
#endif
