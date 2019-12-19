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

#include "MdfConnectManager.h"
#include "MdfThreadManager.h"
#include "MdfLogManager.h"
#include <time.h>

namespace Mdf
{
    //-----------------------------------------------------------------------
    ACE_THR_FUNC_RETURN ThreadReactorFunc(void * arg)
    {
        ACE_Reactor * obj = (ACE_Reactor *)arg;
        obj->run_reactor_event_loop();

        return 0;
    }
    //-----------------------------------------------------------------------
    void DefaultStopSignal(int signo)
    {
        Mlog("receive signal:%d", signo);
        M_Only(ConnectManager)->closeAllServerConnect();
        M_Only(ConnectManager)->closeAllClientConnect();
        M_Only(ConnectManager)->destroyAllReactor();
        ACE_OS::exit(0);
    }
    //-----------------------------------------------------------------------
    M_SingletonImpl(ConnectManager);
    //-----------------------------------------------------------------------
    ServerInfo::ServerInfo():
        mCID(0),
        mExt(0)
    {
    }
    //-----------------------------------------------------------------------
    ServerInfo::ServerInfo(Mui8 type, ConnectID cid) :
        mCID(cid),
        mType(type),
        mExt(0)
    {
    }
    //-----------------------------------------------------------------------
    ServerInfo::~ServerInfo()
    {
        if (mExt)
        {
            delete mExt;
            mExt = 0;
        }
    }
    //-----------------------------------------------------------------------
    bool ServerInfo::operator = (const ServerInfo & o)
    {
        if (mType == o.mType)
        {
            if ((mIP == o.mIP || mIP2 == o.mIP2 || mIP == o.mIP2 || mIP2 == o.mIP) && (mPort == o.mPort))
                return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // ConnectInfo
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ConnectInfo::ConnectInfo(Mui32 id):
        mID(id),
        mConnect(0),
        mConnectState(false)
    {
    }
    //-----------------------------------------------------------------------
    ConnectInfo::~ConnectInfo()
    {
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // ConnectManager
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ConnectManager::ClientReConnect::ClientReConnect(Mui32 ctype) :
        mClientType(ctype)
    {
    }
    //-----------------------------------------------------------------------
    void ConnectManager::ClientReConnect::onTimer(TimeDurMS tick)
    {
        const ClientList & clist = M_Only(ConnectManager)->getClientList(mClientType);
        ClientList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = i->second;
            if (!info->mConnectState)
            {
                info->mCurrentCount++;
                if (info->mCurrentCount < info->mRetryCount)
                {
                    ACE_INET_Addr serIP(info->mServerPort, info->mServerIP.c_str());
                    ACE_Connector<SocketClientPrc, ACE_SOCK_CONNECTOR> connector;
                    if (connector.connect(info->mConnect, serIP) == -1)
                    {
                        info->mConnectState = false;
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // TimerListener
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ConnectManager::TimerListener::TimerListener() : 
        mInterval(50), 
        mEnable(false) {}
    //-----------------------------------------------------------------------
    ConnectManager::TimerListener::~TimerListener() 
    {
        if(mEnable)
            M_Only(ConnectManager)->removeTimer(this);
    }
    //-----------------------------------------------------------------------
    void ConnectManager::TimerListener::setEnable(bool set)
    {
        if (mEnable != set)
        {
            mEnable = set;
            if (mEnable)
                M_Only(ConnectManager)->addTimer(this);
            else
                M_Only(ConnectManager)->removeTimer(this);
        }
    }
    //-----------------------------------------------------------------------
    ConnectManager::ConnectManager():
        mUserCount(0),
        mReactor(0)
    {

    }
    //-----------------------------------------------------------------------
    ConnectManager::~ConnectManager()
    {
        if (mReactor)
        {
            mReactor->cancel_timer(this, 1);
            mReactor->end_reactor_event_loop();
            mReactor = 0;
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::addTimer(TimerListener * obj)
    {
        ScopeLock scope(mTimerListMutex);
        TimerListenerList::iterator i, iend = mTimerList.end();
        for (i = mTimerList.begin(); i != iend; ++i)
        {
            if (*i == obj)
                return;
        }
        mTimerList.push_back(obj);
        obj->mLastTick = getTimeTick();
    }
    //-----------------------------------------------------------------------
    void ConnectManager::removeTimer(TimerListener * obj)
    {
        ScopeLock scope(mTimerListMutex);
        TimerListenerList::iterator i, iend = mTimerList.end();
        for (i = mTimerList.begin(); i != iend; ++i)
        {
            if (*i == obj)
            {
                mTimerList.erase(i);
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::setTimer(bool set, TimeDurMS delay, TimeDurMS interval)
    {
        ScopeLock scope(mTimerListMutex);
        if (set)
        {
            mReactor = new ACE_Reactor(new ACE_TP_Reactor(), true);
            ACE_Time_Value d(0, delay * 1000);
            ACE_Time_Value i(0, interval * 1000);
            mReactor->schedule_timer(this, 0, d, i);
            ACE_Thread_Manager::instance()->spawn_n(1, ThreadReactorFunc, mReactor);
        }
        else
        {
            if (mReactor)
            {
                mReactor->cancel_timer(this, 1);
                mReactor->end_reactor_event_loop();
                mReactor = 0;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::addServerConnect(Mui8 type, ServerIO * obj)
    {
        ScopeLock scope(mSConnectTypeListMutex);
        ServerConnectTypeList::iterator i = mSConnectTypeList.find(type);
        if(i == mSConnectTypeList.end())
        {
            i = mSConnectTypeList.insert(make_pair(type, ServerConnectList())).first;
            i->second.insert(make_pair(obj->getID(), obj));
        }
        else
        {
            ServerConnectList::iterator j = i->second.find(obj);
            if (j == i->second.end())
            {
                i->second.insert(make_pair(obj->getID(), obj));
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::removeServerConnect(Mui8 type, ServerIO * obj)
    {
        ScopeLock scope(mSConnectTypeListMutex);
        ServerConnectTypeList::iterator i = mSConnectTypeList.find(type);
        if (i != mSConnectTypeList.end())
        {
            ServerConnectList::iterator j = i->second.find(obj->getID());
            if (j != i->second.end())
            {
                i->second.erase(j);
                if (i->second.empty())
                {
                    mSConnectTypeList.erase(i);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    ServerIO * ConnectManager::getServerConnect(Mui8 type, ConnectID cid)
    {
        ScopeLock scope(mSConnectTypeListMutex);
        ServerConnectTypeList::iterator i = mSConnectTypeList.find(type);
        if (i != mSConnectTypeList.end())
        {
            ServerConnectList::iterator j = i->second.find(cid);
            if (j != i->second.end())
            {
                return j->second;
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    const ConnectManager::ServerConnectList & ConnectManager::getServerConnectList(Mui8 type) const
    {
        static ServerConnectList temp;
        {
            ScopeLock scope(mSConnectTypeListMutex);
            ServerConnectTypeList::const_iterator i = mSConnectTypeList.find(type);
            if (i != mSConnectTypeList.end())
            {
                return i->second;
            }
        }
        return temp;
    }
    //-----------------------------------------------------------------------
    void ConnectManager::closeAllServerConnect()
    {
        ScopeLock scope(mSConnectTypeListMutex);
        ServerConnectTypeList::iterator i, iend = mSConnectTypeList.end();
        for (i != mSConnectTypeList.begin(); i != iend; ++i)
        {
            ServerConnectList::iterator j, jend = i->second.end();
            for (j = i->second.begin(); j != jend; ++i)
            {
                j->second->stop(); //may be auoto call removeServerConnect
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::addClientConnect(Mui8 type, ClientIO * obj)
    {
        ScopeLock scope(mCConnectTypeListMutex);
        ClientConnectTypeList::iterator i = mCConnectTypeList.find(type);
        if (i == mCConnectTypeList.end())
        {
            i = mCConnectTypeList.insert(make_pair(type, ClientConnectList())).first;
            i->second.insert(make_pair(obj->getID(), obj));
        }
        else
        {
            ClientConnectList::iterator j = i->second.find(obj);
            if (j == i->second.end())
            {
                i->second.insert(make_pair(obj->getID(), obj));
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::removeClientConnect(Mui8 type, ClientIO * obj)
    {
        ScopeLock scope(mCConnectTypeListMutex);
        ClientConnectTypeList::iterator i = mCConnectTypeList.find(type);
        if (i != mCConnectTypeList.end())
        {
            ClientConnectList::iterator j = i->second.find(obj->getID());
            if (j != i->second.end())
            {
                i->second.erase(j);
                if (i->second.empty())
                {
                    mCConnectTypeList.erase(i);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    ClientIO * ConnectManager::getClientConnect(Mui8 type, ConnectID cid)
    {
        ScopeLock scope(mCConnectTypeListMutex);
        ClientConnectTypeList::iterator i = mCConnectTypeList.find(type);
        if (i != mCConnectTypeList.end())
        {
            ClientConnectList::iterator j = i->second.find(cid);
            if (j != i->second.end())
            {
                return j->second;
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    const ConnectManager::ClientConnectList & ConnectManager::getClientConnectList(Mui8 type) const
    {
        static ClientConnectList temp;
        {
            ScopeLock scope(mCConnectTypeListMutex);
            ClientConnectTypeList::const_iterator i = mCConnectTypeList.find(type);
            if (i != mCConnectTypeList.end())
            {
                return i->second;
            }
        }
        return temp;
    }
    //-----------------------------------------------------------------------
    void ConnectManager::closeAllClientConnect()
    {
        ScopeLock scope(mCConnectTypeListMutex);
        ClientConnectTypeList::iterator i, iend = mCConnectTypeList.end();
        for(i = mCConnectTypeList.begin(); i != iend; ++i)
        {
            ClientConnectList::iterator j, jend = i->second.end();
            for (j = i->second.begin(); j != jend; ++j)
            {
                return j->second->stop(); //may be auoto call removeClientConnect
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::sendServerConnect(Mui8 type, Message * msg)
    {
        ScopeLock scope(mSConnectTypeListMutex);
        ServerConnectTypeList::iterator i = mSConnectTypeList.find(type);
        if (i != mSConnectTypeList.end())
        {
            ServerConnectList::iterator j, jend = i->second.end();
            for (j = i->second.begin(); j != jend; ++j)
            {
                j->second->send(msg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::sendClientConnect(Mui8 type, Message * msg)
    {
        ScopeLock scope(mCConnectTypeListMutex);
        ClientConnectTypeList::iterator i = mCConnectTypeList.find(type);
        if (i != mCConnectTypeList.end())
        {
            ClientConnectList::iterator j, jend = i->second.end();
            for (j = i->second.begin(); j != jend; ++j)
            {
                j->second->send(msg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::addServer(Mui8 type, ServerInfo * server)
    {
        ScopeLock scope(mServerTypeListMutex);
        ServerTypeList::iterator i = mServerTypeList.find(type);
        if(i == mServerTypeList.end())
        {
            i = mServerTypeList.insert(make_pair(type, ServerList())).first;
            i->second.insert(make_pair(server->getCID(), server));
            //mUserCount += server->mConnectCount;
        }
        else
        {
            ServerList::iterator j = i->second.find(server->getCID());
            if(j == i->second.end())
            {
                i->second.insert(make_pair(server->getCID(), server));
                //mUserCount += server->mConnectCount;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::removeServer(Mui8 type, ConnectID cid)
    {
        ScopeLock scope(mServerTypeListMutex);
        ServerTypeList::iterator i = mServerTypeList.find(type);
        if(i != mServerTypeList.end())
        {
            ServerList::iterator j = i->second.find(cid);
            if (j != i->second.end())
            {
                //mUserCount -= j->second->mConnectCount;
                delete j->second;
                i->second.erase(j);
                if (i->second.empty())
                {
                    mServerTypeList.erase(i);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    MCount ConnectManager::getServerCount(Mui8 type) const
    {
        ScopeLock scope(mServerTypeListMutex);
        ServerTypeList::const_iterator i = mServerTypeList.find(type);
        if(i != mServerTypeList.end())
        {
            return i->second.size();
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    ServerInfo * ConnectManager::getServer(Mui8 type, ConnectID cid) const
    {
        ScopeLock scope(mServerTypeListMutex);
        ServerTypeList::const_iterator i = mServerTypeList.find(type);
        if (i != mServerTypeList.end())
        {
            ServerList::const_iterator j = i->second.find(cid);
            if (j != i->second.end())
            {
                return j->second;
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    ServerInfo * ConnectManager::getServer(Mui8 type, bool lessconnect) const
    {
        ScopeLock scope(mServerTypeListMutex);
        ServerInfo * re = 0;
        Mui32 curmin = (Mui32)-1;
        ServerTypeList::const_iterator i = mServerTypeList.find(type);
        if(i != mServerTypeList.end())
        {
            ServerList::const_iterator it = i->second.begin();
            ServerList::const_iterator itend = i->second.end();

            if (!i->second.empty() && !lessconnect)
                return it->second;

            for(; it != itend; ++it)
            {
                ServerInfo * temp = it->second;
                if ((temp->mConnectCount < temp->mMaxConnect) && (temp->mConnectCount < curmin))
                {
                    curmin = temp->mConnectCount;
                    re = it->second;
                }
            }
        }
        return re;
    }
    //-----------------------------------------------------------------------
    const ConnectManager::ServerList & ConnectManager::getServerList(Mui8 type) const
    {
        static ServerList temp;
        {
            ScopeLock scope(mServerTypeListMutex);
            ServerTypeList::const_iterator i = mServerTypeList.find(type);
            if (i != mServerTypeList.end())
            {
                return i->second;
            }
        }
        return temp;
    }
    //-----------------------------------------------------------------------
    void ConnectManager::addClient(Mui8 type, ConnectInfo * server)
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::iterator i = mClientTypeList.find(type);
        if (i == mClientTypeList.end())
        {
            i = mClientTypeList.insert(make_pair(type, ClientList())).first;
            i->second.insert(make_pair(server->getID(), server));
            //mUserCount += server->mConnectCount;
        }
        else
        {
            ClientList::iterator j = i->second.find(server->getID());
            if (j == i->second.end())
            {
                i->second.insert(make_pair(server->getID(), server));
                //mUserCount += server->mConnectCount;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::removeClient(Mui8 type, Mui32 id)
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::iterator i = mClientTypeList.find(type);
        if (i != mClientTypeList.end())
        {
            ClientList::iterator j = i->second.find(id);
            if (j != i->second.end())
            {
                //mUserCount -= j->second->mConnectCount;
                delete j->second;
                i->second.erase(j);
                if (i->second.empty())
                {
                    mClientTypeList.erase(i);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    MCount ConnectManager::getClientCount(Mui8 type) const
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::const_iterator i = mClientTypeList.find(type);
        if (i != mClientTypeList.end())
        {
            return i->second.size();
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    void ConnectManager::confirmClient(Mui8 type, Mui32 id)
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::const_iterator i = mClientTypeList.find(type);
        if (i != mClientTypeList.end())
        {
            ClientList::const_iterator j = i->second.find(id);
            if (j != i->second.end())
            {
                ConnectInfo * info = j->second;
                info->mConfirmTime = M_Only(ConnectManager)->getTimeTick();
                info->mConnectState = true;
                info->mRetryCount = M_ReConnect_Min / 2;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::resetClient(Mui8 type, Mui32 id)
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::const_iterator i = mClientTypeList.find(type);
        if (i != mClientTypeList.end())
        {
            ClientList::const_iterator j = i->second.find(id);
            if (j != i->second.end())
            {
                ConnectInfo * info = j->second;
                if (info->mConnect)
                {
                    info->mConnect->setAutoDestroy(false);
                }
                info->mCurrentCount = 0;
                info->mRetryCount *= 2;
                info->mConnectState = false;

                if (info->mRetryCount > M_ReConnect_Max)
                {
                    info->mRetryCount = M_ReConnect_Min;
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    ConnectInfo * ConnectManager::getClient(Mui8 type, Mui32 id) const
    {
        ScopeLock scope(mClientTypeListMutex);
        ClientTypeList::const_iterator i = mClientTypeList.find(type);
        if (i != mClientTypeList.end())
        {
            ClientList::const_iterator j = i->second.find(id);
            if (j != i->second.end())
            {
                return j->second;
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    const ConnectManager::ClientList & ConnectManager::getClientList(Mui8 type) const
    {
        static ClientList temp;
        {
            ScopeLock scope(mClientTypeListMutex);
            ClientTypeList::const_iterator i = mClientTypeList.find(type);
            if (i != mClientTypeList.end())
            {
                return i->second;
            }
        }
        return temp;
    }
    //-----------------------------------------------------------------------
    ACE_Reactor * ConnectManager::createReactor(ACE_TP_Reactor * tp)
    {
        ACE_Reactor * re = new ACE_Reactor(tp, true);
        {
            ScopeLock scope(mReactListMutex);
            mReactList.push_back(re);
        }
        return re;
    }
    //-----------------------------------------------------------------------
    void ConnectManager::spawnReactor(MCount threadcnt, ACE_TP_Reactor * tp, ACE_THR_FUNC mainfunc)
    {
        ACE_Thread_Manager::instance()->spawn_n(threadcnt, mainfunc, tp);
    }
    //-----------------------------------------------------------------------
    void ConnectManager::stopReactor(ACE_TP_Reactor * tp)
    {
        mReactor->end_reactor_event_loop();
    }
    //-----------------------------------------------------------------------
    void ConnectManager::destroyReactor(ACE_Reactor * obj)
    {
        ScopeLock scope(mReactListMutex);
        ReactList::iterator i, iend = mReactList.end();
        for (i = mReactList.begin(); i != iend; ++i)
        {
            if (*i == obj)
            {
                mReactList.erase(i);
                delete *i;
                break;
            }
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::stopAllReactor()
    {
        ScopeLock scope(mReactListMutex);
        ReactList::iterator i, iend = mReactList.end();
        for (i = mReactList.begin(); i != iend; ++i)
        {
            (*i)->end_reactor_event_loop();
        }
    }
    //-----------------------------------------------------------------------
    void ConnectManager::destroyAllReactor()
    {
        ScopeLock scope(mReactListMutex);
        ReactList::iterator i, iend = mReactList.end();
        for (i = mReactList.begin(); i != iend; ++i)
        {
            delete *i;
        }
        mReactList.clear();
    }
    //-----------------------------------------------------------------------
    void ConnectManager::prcMsg(ACE_SOCK_Stream * obj, Message * msg)
    {
        //obj->onMessage(msg);
    }
    //-----------------------------------------------------------------------
    void ConnectManager::setMessagePrc(Mui8 cid, CPrcCB prc)
    {

    }
    //-----------------------------------------------------------------------
    CPrcCB ConnectManager::getMessagePrc(Mui8 cid) const
    {
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 ConnectManager::getTimeTick() const
    {
#ifdef _WIN32
        LARGE_INTEGER liCounter;
        LARGE_INTEGER liCurrent;

        if (!QueryPerformanceFrequency(&liCounter))
            return GetTickCount();

        QueryPerformanceCounter(&liCurrent);
        return (uint64_t)(liCurrent.QuadPart * 1000 / liCounter.QuadPart);
#else
        struct timeval tval;
        uint64_t ret_tick;

        gettimeofday(&tval, NULL);

        ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
        return ret_tick;
#endif
    }
    //-----------------------------------------------------------------------
    int ConnectManager::handle_timeout(const ACE_Time_Value & current_time, const void *)
    {
        Mui64 tick = getTimeTick();
        {
            ScopeLock scope(mTimerListMutex);
            TimerListenerList::iterator i, iend = mTimerList.end();
            for (i = mTimerList.begin(); i != iend; ++i)
            {
                if (tick > (*i)->mLastTick ? (tick - (*i)->mLastTick > (*i)->mInterval):((*i)->mLastTick - tick > (*i)->mInterval))
                {
                    (*i)->onTimer(tick);
                    (*i)->mLastTick = tick;
                }
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------
}