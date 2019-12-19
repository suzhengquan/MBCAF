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
#include "MdfConfigFile.h"

namespace Mdf
{
    //----------------------------------------------------------------
    M_SingletonImpl(DatabaseManager);
    #define _MDF_Database_CountMin 2
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // MysqlResult
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    DatabaseResult::DatabaseResult()
    {

    }
    //----------------------------------------------------------------
    DatabaseResult::~DatabaseResult()
    {

    }
    //----------------------------------------------------------------
    int DatabaseResult::getIndex(const String & key)
    {
        map<String, int>::iterator it = mFieldList.find(key);
        if (it == mFieldList.end())
        {
            return -1;
        }
        else
        {
            return it->second;
        }
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // DatabaseInstance
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    PrepareExec::PrepareExec():
        mParamCount(0)
    {

    }
    //----------------------------------------------------------------
    PrepareExec::~PrepareExec()
    {

    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // DatabaseConnect
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    DatabaseConnect::DatabaseConnect(DatabaseInstance * ins) :
        mInstance(ins)
    {
    }
    //----------------------------------------------------------------
    DatabaseConnect::~DatabaseConnect()
    {
        mInstance = 0;
    }
    //----------------------------------------------------------------
        //----------------------------------------------------------------
    const String & DatabaseConnect::getName() const
    {
        return mInstance->getName();
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // DatabaseInstance
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    DatabaseInstance::DatabaseInstance(const String & name, const String & ip,
        Mui16 port, const String & username, const String & pw, const String & dbname,
        Mui32 connectMax)
    {
        mName = name;
        mIP = ip;
        mPort = port;
        mUserName = username;
        mPassword = pw;
        mDbName = dbname;
        mConnectMax = connectMax;
        mConnectCount = _MDF_Database_CountMin;
    }
    //----------------------------------------------------------------
    DatabaseInstance::~DatabaseInstance()
    {
        list<DatabaseConnect *>::iterator i, iend = mFreeList.end();
        for(i = mFreeList.begin(); i != iend; ++i)
        {
            delete *i;
        }

        mFreeList.clear();
    }
    //----------------------------------------------------------------
    int DatabaseInstance::init()
    {
        for (int i = 0; i < mConnectCount; ++i)
        {
            DatabaseConnect * conn = new DatabaseConnect(this);
            int re = conn->connect();
            if(re)
            {
                delete conn;
                return re;
            }

            mFreeList.push_back(conn);
        }

        Mlog("db pool: %s, size: %d", mName.c_str(), (int)mFreeList.size());
        return 0;
    }
    //----------------------------------------------------------------
    DatabaseConnect * DatabaseInstance::getTempConnect()
    {
        mListCondition.lock();

        while (mFreeList.empty())
        {
            if (mConnectCount >= mConnectMax)
            {
                mListCondition.wait();
            }
            else
            {
                DatabaseConnect * conn = new DatabaseConnect(this);
                int ret = conn->init();
                if (ret)
                {
                    Mlog("init DBConnecton failed");
                    delete conn;
                    mListCondition.unlock();
                    return NULL;
                }
                else
                {
                    mFreeList.push_back(conn);
                    mConnectCount++;
                    Mlog("new db connection: %s, conn_cnt: %d", mName.c_str(), mConnectCount);
                }
            }
        }

        DatabaseConnect * re = mFreeList.front();
        mFreeList.pop_front();

        mListCondition.unlock();

        return re;
    }
    //----------------------------------------------------------------
    void DatabaseInstance::freeTempConnect(DatabaseConnect * pConn)
    {
        mListCondition.lock();

        list<DatabaseConnect *>::iterator i, iend = mFreeList.end();
        for (i = mFreeList.begin(); i != iend; ++i)
        {
            if (*i == pConn)
            {
                break;
            }
        }

        if (i == mFreeList.end())
        {
            mFreeList.push_back(pConn);
        }

        mListCondition.signal();
        mListCondition.unlock();
    }
    //----------------------------------------------------------------
    DatabaseManager::DatabaseManager()
    {
        ConfigFile config_file("server.conf", "Database");
        String databaseList;
        config_file.getValue("DatabaseList", databaseList);

        if (!databaseList)
        {
            Mlog("not configure DatabaseList");
            return 1;
        }

        char host[64];
        char port[64];
        char dbname[64];
        char username[64];
        char password[64];
        char maxconncnt[64];
        StringList dblist;
        StrUtil::split(databaseList, dblist, ',');

        for (Mui32 i = 0; i < dblist.size(); ++i)
        {
            char * databasename = dblist[i];
            snprintf(host, 64, "%s_host", databasename);
            snprintf(port, 64, "%s_port", databasename);
            snprintf(dbname, 64, "%s_dbname", databasename);
            snprintf(username, 64, "%s_username", databasename);
            snprintf(password, 64, "%s_password", databasename);
            snprintf(maxconncnt, 64, "%s_maxconncnt", databasename);

            String dbHost;
            Mui32 dbPort;
            String dbName;
            String db_username;
            String db_password;
            Mui32 maxconnCnt;

            if (!config_file.getValue(host, dbHost) || !config_file.getValue(port, dbPort) ||
                !config_file.getValue(dbname, dbName) || !config_file.getValue(username, db_username) ||
                !config_file.getValue(password, db_password) || !config_file.getValue(maxconncnt, maxconnCnt))
            {
                Mlog("not configure db instance: %s", databasename);
                return 2;
            }

            DatabaseInstance * conn = new DatabaseInstance(databasename, dbHost,
                dbPort, db_username, db_password, dbName, maxconnCnt);
            if (conn->init())
            {
                Mlog("init db instance failed: %s", databasename);
                return 3;
            }
            mInstanceList.insert(make_pair(databasename, conn));
        }
    }
    //----------------------------------------------------------------
    DatabaseManager::~DatabaseManager()
    {
    }
    //----------------------------------------------------------------
    DatabaseConnect * DatabaseManager::getTempConnect(const String & name)
    {
        map<String, DatabaseInstance *>::iterator i = mInstanceList.find(name);
        if (i != mInstanceList.end())
        {
            return i->second->getTempConnect();
        }
        return 0;
    }
    //----------------------------------------------------------------
    void DatabaseManager::freeTempConnect(DatabaseConnect * obj)
    {
        assert(obj)
        map<String, DatabaseInstance *>::iterator i = mInstanceList.find(obj->getName());
        if(i != mInstanceList.end())
        {
            i->second->freeTempConnect(obj);
        }
    }
    //----------------------------------------------------------------
}