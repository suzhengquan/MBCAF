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

#ifndef _MDF_CONNECT_MANAGER_H_
#define _MDF_CONNECT_MANAGER_H_

#include "MdfPreInclude.h"
#include "MdfServerIO.h"
#include "MdfClientIO.h"

namespace Mdf
{
    /** 服务器信息
    @version 0.9.1
    */
    class MdfNetAPI ServerInfo
    {
    public:
        class ServerInfoData
        {
        public:
            ServerInfoData() {}
            virtual ~ServerInfoData() {}
        };
    public:
        ServerInfo();
        ServerInfo(Mui8 type, ConnectID cid);
        ~ServerInfo();

        inline ConnectID getCID() const
        {
            return mCID;
        }

        inline Mui32 getID() const
        {
            return mID ;
        }
        bool operator = (const ServerInfo & o);
    public:
        ConnectID mCID;         ///< 
        Mui8 mType;             ///< 服务器/客服端类型
        Mui8 mPermission;       ///< 是否需要权限
        Mui32 mID;              ///< hash(IP)+hash(IP2)+mPort
        String mIP;             ///<
        String mIP2;            ///<
        Mui16 mPort;            ///<
        Mui32 mMaxConnect;      ///<
        Mui32 mConnectCount;    ///<
        String hostname;        ///< 服务器名
        String mTip;            ///< 登陆提示
        ServerInfoData * mExt;  ///< 辅助数据
    };

    class MdfNetAPI ConnectInfo
    {
    public:
        ConnectInfo(Mui32 id);
        ~ConnectInfo();

        inline Mui32 getID() const
        {
            return mID;
        }
    public:
        Mui32 mID;
        String mServerIP;
        Mui16 mServerPort;
        Mui32 mCurrentCount;
        Mui32 mRetryIterval;
        Mui32 mRetryCount;
        Mui64 mConfirmTime;
        ClientIO * mClientIO;
        SocketClientPrc * mConnect;
        bool mConnectState;
    };

    typedef std::vector<ConnectInfo *> ConnectInfoList;

    MdfNetAPI ACE_THR_FUNC_RETURN ThreadReactorFunc(void * arg);
    
    MdfNetAPI void DefaultStopSignal(int signo);

    /** 连接管理器(Connect Manager)
    @remark 可动态切换信息协议
    @version 0.9.1
    */
    class MdfNetAPI ConnectManager : public ACE_Event_Handler
    {
    public:
        class MdfNetAPI TimerListener
        {
            friend class ConnectManager;
        public:
            TimerListener();
            virtual ~TimerListener();
        protected:
            /**
            @version 0.9.1
            */
            virtual void onTimer(TimeDurMS tick) = 0;

            /**
            @version 0.9.1
            */
            void setEnable(bool set);

            /**
            @version 0.9.1
            */
            inline bool isEnable() const { return mEnable; }

            /**
            @version 0.9.1
            */
            inline void setInterval(TimeDurMS ms) { mInterval = ms; }

            /**
            @version 0.9.1
            */
            inline TimeDurMS getInterval() const { return mInterval; }
        private:
            TimeDurMS mInterval;
            TimeDurMS mLastTick;
            bool mEnable;
        };

        class MdfNetAPI ClientReConnect : public TimerListener
        {
        public:
            ClientReConnect(Mui32 ctype);
        protected:
            /// @copydetails TimerListener::onTimer
            void onTimer(TimeDurMS tick);
        private:
            Mui32 mClientType;
        };
        typedef std::vector<TimerListener *> TimerListenerList;
    public:
        typedef std::map<ConnectID, ServerIO *> ServerConnectList;
        typedef std::map<Mui8, ServerConnectList> ServerConnectTypeList;
        typedef std::map<ConnectID, ClientIO *> ClientConnectList;
        typedef std::map<Mui8, ClientConnectList> ClientConnectTypeList;
        typedef std::map<ConnectID, ServerInfo *> ServerList;
        typedef std::map<Mui8, ServerList> ServerTypeList;
        typedef std::map<Mi32, ConnectInfo *> ClientList;
        typedef std::map<Mui8, ClientList> ClientTypeList;
    public:
        ConnectManager();
        ~ConnectManager();

        /**
        @version 0.9.1
        */
        void addTimer(TimerListener * obj);

        /**
        @version 0.9.1
        */
        void removeTimer(TimerListener * obj);

        /**
        @version 0.9.1
        */
        void setTimer(bool set, TimeDurMS delay, TimeDurMS interval);

        /**
        @version 0.9.1
        */
        void addServerConnect(Mui8 type, ServerIO * obj);
        
        /**
        @version 0.9.1
        */
        void removeServerConnect(Mui8 type, ServerIO * obj);

        /**
        @version 0.9.1
        */
        ServerIO * getServerConnect(Mui8 type, ConnectID cid);

        /**
        @version 0.9.1
        */
        const ServerConnectList & getServerConnectList(Mui8 type) const;

        /**
        @version 0.9.1
        */
        void closeAllServerConnect();

        /**
        @version 0.9.1
        */
        void addClientConnect(Mui8 type, ClientIO * obj);

        /**
        @version 0.9.1
        */
        void removeClientConnect(Mui8 type, ClientIO * obj);

        /**
        @version 0.9.1
        */
        ClientIO * getClientConnect(Mui8 type, ConnectID cid);

        /**
        @version 0.9.1
        */
        const ClientConnectList & getClientConnectList(Mui8 type) const;

        /**
        @version 0.9.1
        */
        void closeAllClientConnect();

        /**
        @version 0.9.1
        */
        void sendServerConnect(Mui8 type, Message * msg);

        /**
        @version 0.9.1
        */
        void sendClientConnect(Mui8 type, Message * msg);

        /**
        @version 0.9.1
        */
        void addServer(Mui8 type, ServerInfo * server);

        /**
        @version 0.9.1
        */
        void removeServer(Mui8 type, ConnectID id);

        /**
        @version 0.9.1
        */
        MCount getServerCount(Mui8 type) const;

        /**
        @version 0.9.1
        */
        ServerInfo * getServer(Mui8 type, ConnectID id) const;

        /**
        @version 0.9.1
        */
        ServerInfo * getServer(Mui8 type, bool lessconnect) const;

        /**
        @version 0.9.1
        */
        const ServerList & getServerList(Mui8 type) const;

        /**
        @version 0.9.1
        */
        void addClient(Mui8 type, ConnectInfo * server);

        /**
        @version 0.9.1
        */
        void removeClient(Mui8 type, Mui32 id);

        /**
        @version 0.9.1
        */
        MCount getClientCount(Mui8 type) const;

        /**
        @version 0.9.1
        */
        void confirmClient(Mui8 type, Mui32 id);

        /**
        @version 0.9.1
        */
        void resetClient(Mui8 type, Mui32 id);

        /**
        @version 0.9.1
        */
        ConnectInfo * getClient(Mui8 type, Mui32 id) const;

        /**
        @version 0.9.1
        */
        const ClientList & getClientList(Mui8 type) const;

        /**
        @version 0.9.1
        */
        ACE_Reactor * createReactor(ACE_TP_Reactor * tp);

        /**
        @version 0.9.1
        */
        void spawnReactor(MCount threadcnt, ACE_TP_Reactor * tp, ACE_THR_FUNC mainfunc = ThreadReactorFunc);

        /**
        @version 0.9.1
        */
        void stopReactor(ACE_TP_Reactor * tp);

        /**
        @version 0.9.1
        */
        void stopAllReactor();

        /**
        @version 0.9.1
        */
        void destroyReactor(ACE_Reactor * obj);

        /**
        @version 0.9.1
        */
        void destroyAllReactor();

        /**
        @version 0.9.1
        */
        void prcMsg(ACE_SOCK_Stream * obj, Message * msg);

        /**
        @version 0.9.1
        */
        void setMessagePrc(Mui8 cid, CPrcCB prc);

        /**
        @version 0.9.1
        */
        CPrcCB getMessagePrc(Mui8 cid) const;

        /**
        @version 0.9.1
        */
        Mui64 getTimeTick() const;

        /// @copydetails ACE_Event_Handler::handle_timeout
        int handle_timeout(const ACE_Time_Value & current, const void * = 0);
    protected:
        typedef hash_map<Mui32, CPrcCB> PrcList;
        typedef vector<ACE_Reactor *> ReactList;
    protected:
        ACE_Reactor * mReactor;
        PrcList mPrcList;
        ReactList mReactList;
        ServerTypeList mServerTypeList;
        ClientTypeList mClientTypeList;
        ServerConnectTypeList mSConnectTypeList;
        ClientConnectTypeList mCConnectTypeList;
        TimerListenerList mTimerList;
        ACE_Thread_Mutex mReactListMutex;
        mutable ACE_Thread_Mutex mServerTypeListMutex;
        mutable ACE_Thread_Mutex mClientTypeListMutex;
        mutable ACE_Thread_Mutex mSConnectTypeListMutex;
        mutable ACE_Thread_Mutex mCConnectTypeListMutex;
        ACE_Thread_Mutex mTimerListMutex;
        Mui32 mUserCount;
    };
    M_SingletonDef(ConnectManager);
}
#endif