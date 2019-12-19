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

#include "MdfMembaseManager.h"
#include "MdfConfigFile.h"
#include "hiredis.h"

namespace Mdf
{
    M_SingletonImpl(MembaseManager);
    //--------------------------------------------------------------------
    #define _MDF_Membase_ConnectMin 2
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    // MembaseConnect
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    MembaseConnect::MembaseConnect(MembaseInstance * instance)
    {
        mInstance = instance;
        mContext = NULL;
        mLastConnect = 0;
        mInit = false;
    }
    //--------------------------------------------------------------------
    MembaseConnect::~MembaseConnect()
    {
        if (mContext)
        {
            redisFree(mContext);
            mContext = NULL;
        }
    }
    //--------------------------------------------------------------------
    int MembaseConnect::connect()
    {
        if (!mInit)
        {
            uint64_t cur_time = (uint64_t)time(NULL);
            if (cur_time < mLastConnect + 4)
            {
                mInit = false;
                return 1;
            }

            mLastConnect = cur_time;

            struct timeval timeout = { 0, 200000 };
            mContext = redisConnectWithTimeout(mInstance->getIP(), mInstance->getPort(), timeout);
            if (!mContext || mContext->err)
            {
                if (mContext)
                {
                    Mlog("redisConnect failed: %s", mContext->errstr);
                    redisFree(mContext);
                    mContext = NULL;
                    mInit = false;
                }
                else
                {
                    Mlog("redisConnect failed");
                }

                return 1;
            }

            redisReply * reply = (redisReply *)redisCommand(mContext, "SELECT %d", mInstance->getNum());
            if (reply && (reply->type == REDIS_REPLY_STATUS) && (strncmp(reply->str, "OK", 2) == 0))
            {
                freeReplyObject(reply);
                mInit = true;
                return 0;
            }
            else
            {
                Mlog("select cache db failed");
                mInit = false;
                return 2;
            }
        }
        return 0;
    }
    //--------------------------------------------------------------------
    const String & MembaseConnect::getName()
    {
        return mInstance->getName();
    }
    //--------------------------------------------------------------------
    String MembaseConnect::GET(const String & key)
    {
        String value;

        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "GET %s", key.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return value;
            }

            if (reply->type == REDIS_REPLY_STRING)
            {
                value.append(reply->str, reply->len);
            }

            freeReplyObject(reply);
        }
        return value;
    }
    //--------------------------------------------------------------------
    String MembaseConnect::SETEX(const String & key, int timeout, const String & value)
    {
        String ret_value;

        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "SETEX %s %d %s", key.c_str(), timeout, value.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return ret_value;
            }

            ret_value.append(reply->str, reply->len);
            freeReplyObject(reply);
        }
        return ret_value;
    }
    //--------------------------------------------------------------------
    String MembaseConnect::SET(const String & key, const String & value)
    {
        String ret_value;

        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "SET %s %s", key.c_str(), value.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return ret_value;
            }

            ret_value.append(reply->str, reply->len);
            freeReplyObject(reply);
        }
        return ret_value;
    }
    //--------------------------------------------------------------------
    bool MembaseConnect::MGET(const vector<String> & keys, map<String, String> & out)
    {
        if (mInit)
        {
            if (keys.empty())
            {
                return false;
            }

            String strKey;
            bool bFirst = true;
            for (vector<String>::const_iterator it = keys.begin(); it != keys.end(); ++it)
            {
                if (bFirst)
                {
                    bFirst = false;
                    strKey = *it;
                }
                else
                {
                    strKey += " " + *it;
                }
            }

            if (strKey.empty())
            {
                return false;
            }
            strKey = "MGET " + strKey;
            redisReply * reply = (redisReply*)redisCommand(mContext, strKey.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return false;
            }
            if (reply->type == REDIS_REPLY_ARRAY)
            {
                for (size_t i = 0; i < reply->elements; ++i)
                {
                    redisReply* child_reply = reply->element[i];
                    if (child_reply->type == REDIS_REPLY_STRING)
                    {
                        out[keys[i]] = child_reply->str;
                    }
                }
            }
            freeReplyObject(reply);
            return true;
        }
        return false;
    }
    //--------------------------------------------------------------------
    bool MembaseConnect::EXISTS(const String & key)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "EXISTS %s", key.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                return false;
            }
            long ret_value = reply->integer;
            freeReplyObject(reply);
            if (0 == ret_value)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        return false;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::HDEL(const String & key, const String & field)
    {
        if (mInit)
        {
            redisReply* reply = (redisReply *)redisCommand(mContext, "HDEL %s %s", key.c_str(), field.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return 0;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return 0;
    }
    //--------------------------------------------------------------------
    String MembaseConnect::HGET(const String & key, const String & field)
    {
        String ret_value;
        if (mInit)
        {
            redisReply* reply = (redisReply *)redisCommand(mContext, "HGET %s %s", key.c_str(), field.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return ret_value;
            }

            if (reply->type == REDIS_REPLY_STRING)
            {
                ret_value.append(reply->str, reply->len);
            }

            freeReplyObject(reply);
        }
        return ret_value;
    }
    //--------------------------------------------------------------------
    bool MembaseConnect::HGETALL(const String & key, map<String, String> & out)
    {
        if (mInit)
        {
            redisReply* reply = (redisReply *)redisCommand(mContext, "HGETALL %s", key.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return false;
            }

            if ((reply->type == REDIS_REPLY_ARRAY) && (reply->elements % 2 == 0))
            {
                for (size_t i = 0; i < reply->elements; i += 2)
                {
                    redisReply * field_reply = reply->element[i];
                    redisReply * value_reply = reply->element[i + 1];

                    String field(field_reply->str, field_reply->len);
                    String value(value_reply->str, value_reply->len);
                    out.insert(make_pair(field, value));
                }
            }

            freeReplyObject(reply);
            return true;
        }
        return false;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::HSET(const String & key, const String & field, const String & value)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::HINCRBY(const String & key, const String & field, long value)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "HINCRBY %s %s %ld", key.c_str(), field.c_str(), value);
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::INCRBY(const String & key, long value)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply*)redisCommand(mContext, "INCRBY %s %ld", key.c_str(), value);
            if (!reply)
            {
                Mlog("redis Command failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }
            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    String MembaseConnect::HMSET(const String & key, map<String, String> & hash)
    {
        String ret_value;

        if (mInit)
        {
            int argc = hash.size() * 2 + 2;
            const char** argv = new const char*[argc];
            if (!argv)
            {
                return ret_value;
            }

            argv[0] = "HMSET";
            argv[1] = key.c_str();
            int i = 2;
            for (map<String, String>::iterator it = hash.begin(); it != hash.end(); ++it)
            {
                argv[i++] = it->first.c_str();
                argv[i++] = it->second.c_str();
            }

            redisReply* reply = (redisReply *)redisCommandArgv(mContext, argc, argv, NULL);
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                delete[] argv;

                redisFree(mContext);
                mContext = NULL;
                return ret_value;
            }

            ret_value.append(reply->str, reply->len);

            delete[] argv;
            freeReplyObject(reply);
        }
        return ret_value;
    }
    //--------------------------------------------------------------------
    bool MembaseConnect::HMGET(const String & key, const list<String> & fields, list<String> & out)
    {
        if (mInit)
        {
            int argc = fields.size() + 2;
            const char** argv = new const char*[argc];
            if (!argv)
            {
                return false;
            }

            argv[0] = "HMGET";
            argv[1] = key.c_str();
            int i = 2;
            for (list<String>::const_iterator it = fields.begin(); it != fields.end(); ++it)
            {
                argv[i++] = it->c_str();
            }

            redisReply * reply = (redisReply *)redisCommandArgv(mContext, argc, (const char**)argv, NULL);
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                delete[] argv;

                redisFree(mContext);
                mContext = NULL;

                return false;
            }

            if (reply->type == REDIS_REPLY_ARRAY)
            {
                for (size_t i = 0; i < reply->elements; ++i)
                {
                    redisReply* value_reply = reply->element[i];
                    String value(value_reply->str, value_reply->len);
                    out.push_back(value);
                }
            }

            delete[] argv;
            freeReplyObject(reply);
            return true;
        }
        return false;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::INCR(const String & key)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply*)redisCommand(mContext, "INCR %s", key.c_str());
            if (!reply)
            {
                Mlog("redis Command failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }
            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::DECR(const String & key)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply*)redisCommand(mContext, "DECR %s", key.c_str());
            if (!reply)
            {
                Mlog("redis Command failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }
            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::LPUSH(const String & key, const String & value)
    {
        if (mInit)
        {
            redisReply* reply = (redisReply *)redisCommand(mContext, "LPUSH %s %s", key.c_str(), value.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::RPUSH(const String & key, const String & value)
    {
        if (mInit)
        {
            redisReply* reply = (redisReply *)redisCommand(mContext, "RPUSH %s %s", key.c_str(), value.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }

        return -1;
    }
    //--------------------------------------------------------------------
    long MembaseConnect::LLEN(const String & key)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "LLEN %s", key.c_str());
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return -1;
            }

            long ret_value = reply->integer;
            freeReplyObject(reply);
            return ret_value;
        }
        return -1;
    }
    //--------------------------------------------------------------------
    bool MembaseConnect::LRANGE(const String & key, long start, long end, list<String> & ret_value)
    {
        if (mInit)
        {
            redisReply * reply = (redisReply *)redisCommand(mContext, "LRANGE %s %d %d", key.c_str(), start, end);
            if (!reply)
            {
                Mlog("redisCommand failed:%s", mContext->errstr);
                redisFree(mContext);
                mContext = NULL;
                return false;
            }

            if (reply->type == REDIS_REPLY_ARRAY)
            {
                for (size_t i = 0; i < reply->elements; ++i)
                {
                    redisReply * value_reply = reply->element[i];
                    String value(value_reply->str, value_reply->len);
                    ret_value.push_back(value);
                }
            }

            freeReplyObject(reply);
            return true;
        }
        return false;
    }
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    // MembaseInstance
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    MembaseInstance::MembaseInstance(const String & name, const String & ip, Mui32 port,
        Mui32 dbno, Mui32 connectMax)
    {
        mName = name;
        mIP = ip;
        mPort = port;
        mDbNo = dbno;
        mConnectMax = connectMax;
        mConnectCount = _MDF_Membase_ConnectMin;
    }
    //--------------------------------------------------------------------
    MembaseInstance::~MembaseInstance()
    {
        mListCondition.lock();
        list<MembaseConnect*>::iterator it, itend = mFreeList.end();
        for (it = mFreeList.begin(); it != itend; ++it)
        {
            delete *it;
        }

        mFreeList.clear();
        mConnectCount = 0;
        mListCondition.unlock();
    }
    //--------------------------------------------------------------------
    int MembaseInstance::init()
    {
        for (int i = 0; i < mConnectCount; ++i)
        {
            MembaseConnect* conn = new MembaseConnect(this);
            if (conn->connect())
            {
                delete conn;
                return 1;
            }

            mFreeList.push_back(conn);
        }

        Mlog("cache pool: %s, list size: %lu", mName.c_str(), mFreeList.size());
        return 0;
    }
    //--------------------------------------------------------------------
    MembaseConnect * MembaseInstance::getTempConnect() const
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
                MembaseConnect * conn = new MembaseConnect(this);
                int ret = conn->connect();
                if(ret)
                {
                    Mlog("init MembaseConnect failed");
                    delete conn;
                    mListCondition.unlock();
                    return NULL;
                }
                else
                {
                    mFreeList.push_back(conn);
                    mConnectCount++;
                    Mlog("new cache connection: %s, conn_cnt: %d", mName.c_str(), mConnectCount);
                }
            }
        }

        MembaseConnect * conn = mFreeList.front();
        mFreeList.pop_front();

        mListCondition.unlock();

        return conn;
    }
    //--------------------------------------------------------------------
    void MembaseInstance::freeTempConnect(MembaseConnect * obj)
    {
        mListCondition.lock();

        list<MembaseConnect*>::const_iterator it = mFreeList.begin();
        for (; it != mFreeList.end(); ++it)
        {
            if (*it == obj)
            {
                break;
            }
        }

        if (it == mFreeList.end())
        {
            mFreeList.push_back(obj);
        }

        mListCondition.signal();
        mListCondition.unlock();
    }
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    // MembaseManager
    //--------------------------------------------------------------------
    //--------------------------------------------------------------------
    MembaseManager::MembaseManager()
    {
        ConfigFile config_file("server.conf", "Database");
        String membaseList;
        if (!config_file.getValue("MembaseList", membaseList))
        {
            Mlog("not configure CacheIntance");
            return 1;
        }

        char host[64];
        char port[64];
        char db[64];
        char maxconncnt[64];
        StringList instances_name;
        StrUtil::split(membaseList, instances_name, ',');
        for (Mui32 i = 0; i < instances_name.size(); ++i)
        {
            char * membasename = instances_name[i];
            snprintf(host, 64, "%s_host", membasename);
            snprintf(port, 64, "%s_port", membasename);
            snprintf(db, 64, "%s_db", membasename);
            snprintf(maxconncnt, 64, "%s_maxconncnt", membasename);

            String membasehost;
            Mui16 membaseport;
            Mui16 membaseno;
            Mui16 membasemax;
            if (!config_file.getValue(host, membasehost) || !config_file.getValue(port, membaseport) ||
                !config_file.getValue(db, membaseno) || !config_file.getValue(maxconncnt, membasemax))
            {
                Mlog("not configure cache instance: %s", membasename);
                return 2;
            }

            MembaseInstance * re = new MembaseInstance(membasename, membasehost, membaseport,
                membaseno, membasemax);
            if (re->init())
            {
                Mlog("init cache pool failed");
                return 3;
            }

            mInstanceList.insert(make_pair(membasename, re));
        }

        return 0;
    }
    //--------------------------------------------------------------------
    MembaseManager::~MembaseManager()
    {

    }
    //--------------------------------------------------------------------
    MembaseConnect * MembaseManager::getTempConnect(const String & name)
    {
        map<String, MembaseInstance *>::iterator it = mInstanceList.find(name);
        if (it != mInstanceList.end())
        {
            return it->second->getTempConnect();
        }
        else
        {
            return 0;
        }
    }
    //--------------------------------------------------------------------
    void MembaseManager::freeTempConnect(MembaseConnect * obj)
    {
        if (obj)
        {

            map<String, MembaseInstance *>::iterator it = mInstanceList.find(obj->getName());
            if (it != mInstanceList.end())
            {
                return it->second->freeTempConnect(obj);
            }
        }
    }
    //--------------------------------------------------------------------
}