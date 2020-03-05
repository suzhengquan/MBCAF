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

#include "MdfFileClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfFileManager.h"
#include "MdfServerConnect.h"
#include "MdfConnect.h"
#include "MdfStrUtil.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.HubServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    static ClientReConnect * gFileTimer = 0;
    //-----------------------------------------------------------------------
    void setupFileConnect(const ConnectInfoList & clist)
    {
        if (clist.size() == 0)
            return;

        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
        ConnectInfoList::const_iterator i, iend = clist.end();
        for (i = clist.begin(); i != iend; ++i)
        {
            ConnectInfo * info = *i;
            info->mReactor = reactor;
            info->mConnect = new SocketClientPrc(new FileClientConnect(info->mID), reactor);
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
            M_Only(ConnectManager)->addClient(ClientType_File, info);
        }
        M_Only(ConnectManager)->spawnReactor(2, reactor);

        gFileTimer = new ClientReConnect(ClientType_File);
        gFileTimer->setEnable(true);
    }
    //-----------------------------------------------------------------------
    void shutdownFileConnect()
    {
        if(gFileTimer)
        {
            delete gFileTimer;
            gFileTimer = 0;
        }
    }
    //-----------------------------------------------------------------------
    FileClientConnect::FileClientConnect(ACE_Reactor * tor) :
        ClientConnect(tor)
    {
    }
    //-----------------------------------------------------------------------
    FileClientConnect::~FileClientConnect()
    {
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::connect(const String & ip, Mui16 port)
    {
        M_ClientConnect<SocketClientPrc>(react, this, ip, port);
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::onConfirm()
    {
        setTimer(true, 0, 1000);

        MBCAF::HubServer::FileServerQ proto;
        MdfMessage remsg;
		remsg.setProto(&proto);
		remsg.setCommandID(SBMSG(FileServerQ));
        send(&remsg);
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::onTimer(TimeDurMS tick)
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
            Mlog("conn to file server timeout ");
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID()) 
        {
        case SBID_Heartbeat:
            break;
        case SBID_FileTransferA:
            prcFileTransferA(temp);
            break;
        case SBID_FileServerA:
            prcFileServerA(temp);
            break;
        default:
            Mlog("unknown cmd id=%d ", temp->getCommandID());
            break;
        }
    }
    //-----------------------------------------------------------------------
    const list<MBCAF::Proto::IPAddress> & GetFileServerIPList() 
    { 
        return mServerList; 
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::prcFileTransferA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileTransferA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 result = proto.result_code();
        Mui32 fromid = proto.from_user_id();
        Mui32 toid = proto.to_user_id();
        String fname = proto.file_name();
        Mui32 fsize = proto.file_size();
        String task_id = proto.task_id();
        Mui32 trans_mode = proto.trans_mode();
        HandleExtData attach((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mlog("HandleFileMsgTransRsp, result: %u, from_user_id: %u, to_user_id: %u, file_name: %s, \
        task_id: %s, trans_mode: %u. ", result, fromid, toid,
            fname.c_str(), task_id.c_str(), trans_mode);

        const list<MBCAF::Proto::IPAddress> & addrlist = mServerList;

        MBCAF::HubServer::FileA proto2;
        proto2.set_result_code(result);
        proto2.set_from_user_id(fromid);
        proto2.set_to_user_id(toid);
        proto2.set_file_name(fname);
        proto2.set_task_id(task_id);
        proto2.set_trans_mode((MBCAF::Proto::TransferFileType)trans_mode);
        for (list<MBCAF::Proto::IPAddress>::const_iterator it = addrlist.begin(); it != addrlist.end(); it++)
        {
            MBCAF::Proto::IPAddress ip_addr_tmp = *it;
            MBCAF::Proto::IPAddress* ip_addr = proto2.add_ip_addr_list();
            ip_addr->set_ip(ip_addr_tmp.ip());
            ip_addr->set_port(ip_addr_tmp.port());
        }
        MdfMessage remsg;
		remsg.setProto(&proto2);
		remsg.setCommandID(HSMSG(FileA));
		remsg.setSeqIdx(msg->getSeqIdx());
        Mui32 handle = attach.getHandle();

        ServerConnect* fromconn = M_Only(UserManager)->getMsgConnect(fromid, handle);
        if (fromconn)
        {
            fromconn->send(&remsg);
        }

        if (result == 0)
        {
            MBCAF::HubServer::FileNotify proto3;
            proto3.set_from_user_id(fromid);
            proto3.set_to_user_id(toid);
            proto3.set_file_name(fname);
            proto3.set_file_size(file_size);
            proto3.set_task_id(task_id);
            proto3.set_trans_mode((MBCAF::Proto::TransferFileType)trans_mode);
            proto3.set_offline_ready(0);
            for (list<MBCAF::Proto::IPAddress>::const_iterator it = addrlist.begin(); it != addrlist.end(); it++)
            {
                MBCAF::Proto::IPAddress ip_addr_tmp = *it;
                MBCAF::Proto::IPAddress* ip_addr = proto3.add_ip_addr_list();
                ip_addr->set_ip(ip_addr_tmp.ip());
                ip_addr->set_port(ip_addr_tmp.port());
            }
            MdfMessage pdu2;
            pdu2.setProto(&proto3);
            pdu2.setCommandID(HSMSG(FileNotify));

            User * touser = M_Only(UserManager)->getUser(toid);
            if (touser)
            {
                touser->broadcastToPC(&pdu2);
            }

            RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
            if (routeconn)
            {
                routeconn->send(&pdu2);
            }
        }
    }
    //-----------------------------------------------------------------------
    void FileClientConnect::prcFileServerA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileServerA proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 cnt = proto.ip_addr_list_size();

        for(Mui32 i = 0; i < cnt; i++)
        {
            MBCAF::Proto::IPAddress ipaddr = proto.ip_addr_list(i);
            Mlog("prcFileServerA -> %s : %d ", ipaddr.ip().c_str(), ipaddr.port());
            mServerList.push_back(ipaddr);
        }
    }
    //-----------------------------------------------------------------------
    FileClientConnect * FileClientConnect::getRandomConnect()
    {
        Mui32 sercnt = M_Only(ConnectManager)->getClientCount(ClientType_File);
        if (0 == sercnt)
        {
            return 0;
        }
        srand(time(NULL));
        FileClientConnect * re = 0;
        const ConnectManager::ClientList & clist = M_Only(ConnectManager)->getClientList(ClientType_File);
        Mui32 rnum = rand() % sercnt;
        ConnectManager::ClientList::const_iterator i = clist.begin();
        std::advance(i, rnum);
        ConnectInfo * info = i->second;
        if (info->mConnectState)
        {
            re = static_cast<FileClientConnect *>(info->mConnect->getBase());
        }
        else
        {
            for (Mui32 c = 0; c < sercnt; ++c)
            {
                i = clist.begin();
                int j = (rnum + c + 1) % sercnt;
                std::advance(i, rnum);
                info = i->second;
                if (info->mConnectState)
                {
                    re = static_cast<FileClientConnect *>(info->mConnect->getBase());
                    break;
                }
            }
        }
        return re;
    }
    //-----------------------------------------------------------------------
}