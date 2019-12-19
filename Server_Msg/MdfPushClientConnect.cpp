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

#include "MdfPushClientConnect.h"
#include "MdfConnectManager.h"
#include "MdfUserManager.h"
#include "MdfConnect.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.Proto.pb.h"

using namespace MBCAF::Proto;

#define AlterStrMaxLength 40

namespace Mdf
{
    //------------------------------------------------------------------------
    PushClientConnect * PushClientConnect::mPrimaryConnect = NULL
    static ClientReConnect * gPushTimer = 0;
    //------------------------------------------------------------------------
    void setupPushConnect(const ConnectInfoList & clist)
    {
        if (clist.size() == 0)
            return;

        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
        ConnectInfoList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = *i;
            info->mReactor = reactor;
            info->mConnect = new SocketClientPrc(new PushClientConnect(info->mID), reactor);
            ACE_INET_Addr serIP(info->mServerPort, info->mServerIP.c_str());
            ACE_Connector<SocketClientPrc, ACE_SOCK_CONNECTOR> connector;
            if (connector.connect(info->mConnect, serIP) == -1)
            {
                info->mConnectState = false;
            }
            else
            {
                info->mCurrentCount = 0;
            }
            M_Only(ConnectManager)->addClient(ClientType_Push, info);
        }
        M_Only(ConnectManager)->spawnReactor(2, reactor);

        gPushTimer = new ClientReConnect(ClientType_Push);
        gPushTimer->setEnable(true);
    }
    //-----------------------------------------------------------------------
    void shutdownPushConnect()
    {
        if (gPushTimer)
        {
            delete gPushTimer;
            gPushTimer = 0;
        }
    }
    //------------------------------------------------------------------------
    PushClientConnect::PushClientConnect(Mui32 idx):
        mIndex(idx)
    {
    }
    //------------------------------------------------------------------------
    PushClientConnect::~PushClientConnect()
    {

    }
    //------------------------------------------------------------------------
    void PushClientConnect::connect(const String & ip, Mui16 port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //------------------------------------------------------------------------
    void PushClientConnect::onConfirm()
    {
        ClientIO::onConfirm();
        M_Only(ConnectManager)->addClientConnect(ClientType_Push, this);
        M_Only(ConnectManager)->confirmClient(ClientType_Push, mIndex);
        setTimer(true, 0, 1000);

        mPrimaryConnect = this;
    }
    //------------------------------------------------------------------------
    void PushClientConnect::onClose()
    {
        M_Only(ConnectManager)->removeClientConnect(ClientType_Push, this);
        M_Only(ConnectManager)->resetClient(ClientType_Push, mIndex);

        mPrimaryConnect = 0;
    }
    //------------------------------------------------------------------------
    void PushClientConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mSendMark + M_Heartbeat_Interval)
        {
            MBCAF::ServerBase::Heartbeat proto;
            MdfMessage remsg;
            remsg.setProto(&proto);
            remsg.setCommandID(SBMSG(Heartbeat));
            send(&remsg);
        }

        if (tick > mReceiveMark + M_Server_Timeout)
        {
            Mlog("conn to push server timeout ");
            stop();
        }
    }
    //------------------------------------------------------------------------
    void PushClientConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            break;
        case SBID_UserPushA:
            prcUserPushA(temp);
            break;
        default:
            Mlog("push server, wrong cmd id=%d ", temp->getCommandID());
        }
    }
    //------------------------------------------------------------------------
    void PushClientConnect::prcUserPushA(MdfMessage * msg)
    {
    }
    //------------------------------------------------------------------------
    void PushClientConnect::genAlter(String & flash, Mui32 msg_type, Mui32 from_id)
    {
        String pic_prefix = "&$#@~^@[{:";
        String pic_suffix = ":}]&$~@#@";
        size_t pos_prefix = flash.find(pic_prefix);
        size_t pos_suffix = flash.find(pic_suffix);

        String comm_flash = "you receive a message";
        if (pos_prefix != String::npos && pos_suffix != String::npos && pos_prefix < pos_suffix)
        {
            flash = comm_flash;
        }
        else
        {
            User* userobj = M_Only(UserManager)->getUser(from_id);
            if (userobj)
            {
                String nick_name = userobj->getNick();
                String msg_tmp;
                if (msg_type == MBCAF::Proto::MT_GroupAudio)
                {
                    msg_tmp.append(nick_name);
                    msg_tmp.append("send a group voice");
                }
                else if (msg_type == MBCAF::Proto::MT_Audio)
                {
                    msg_tmp.append(nick_name);
                    msg_tmp.append("send a vioce to you");
                }
                else
                {
                    msg_tmp.append(nick_name);
                    msg_tmp.append(":");
                    msg_tmp.append(flash);
                }
                flash = msg_tmp;
                Mui32 maxlength = AlterStrMaxLength - 3;
                if (flash.length() > maxlength)
                {
                    String flash_tmp = flash.substr(0, maxlength);
                    flash_tmp.append("...");
                    flash = flash_tmp;
                }
            }
            else
            {
                flash = comm_flash;
            }
        }
    }
    //------------------------------------------------------------------------
    PushClientConnect * PushClientConnect::getPrimaryConnect()
    {
        if (mPrimaryConnect && mPrimaryConnect->isOpen())
        {
            return mPrimaryConnect;
        }
        return 0;
    }
    //------------------------------------------------------------------------
}