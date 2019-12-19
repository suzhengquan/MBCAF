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

#include "MdfMsgServerConnect.h"
#include "MdfTransferTask.h"
#include "MdfTransferManager.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    MsgServerConnect::MsgServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    MsgServerConnect::~MsgServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    ServerIO * MsgServerConnect::createInstance() const
    {
        return new MsgServerConnect();
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::onConnect()
    {
        M_Only(ConnectManager)->addServerConnect(ServerType_MsgClient, this);
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::onClose()
    {
        M_Only(ConnectManager)->removeServerConnect(ServerType_MsgClient, this);
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mReceiveMark + M_Server_Timeout)
        {
            Mlog("Msg server timeout");
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            prcHeartbeat(temp);
            break;
        case SBID_FileTransferQ:
            prcFileTransferA(temp);
            break;
        case SBID_FileServerQ:
            PrcServerAddressA(temp);
            break;
        default:
            Mlog("No such cmd id = %u", temp->getCommandID());
            break;
        }
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::prcHeartbeat(MdfMessage * msg)
    {
        send(msg);
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::prcFileTransferA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileTransferQ ftq;
        if(!msg->toProto(&ftq))
            return;

        Mui32 from_id = ftq.getFromID();
        Mui32 to_id = ftq.getToID();

        MBCAF::HubServer::FileTransferA fta;
        fta.set_result_code(1);
        fta.setFromID(from_id);
        fta.setToID(to_id);
        fta.setFileName(ftq.getFileName());
        fta.setFileSize(ftq.getFileSize());
        fta.setID("");
        fta.set_trans_mode(ftq.trans_mode());
        fta.set_attach_data(ftq.attach_data());

        bool rv = false;
        do
        {
            std::String tid = GenerateUUID();
            if (tid.empty())
            {
                Mlog("Create task id failed");
                break;
            }
            Mlog("trams_mode=%d, tid=%s, from_id=%d, to_id=%d, file_name=%s, getFileSize=%d", ftq.trans_mode(), tid.c_str(), from_id, to_id, ftq.getFileName().c_str(), ftq.getFileSize());

            TransferTask * transfer_task = M_Only(TransferManager)->createTask(
                ftq.trans_mode(),
                tid,
                from_id,
                to_id,
                ftq.getFileName(),
                ftq.getFileSize());

            if (transfer_task == NULL)
            {
                Mlog("Create task failed");
                break;
            }

            fta.set_result_code(0);
            fta.setID(tid);
            rv = true;

            Mlog("Create task succeed, task id %s, task type %d, from user %d, to user %d", tid.c_str(), ftq.trans_mode(), from_id, to_id);
        } while (0);

        send(MTID_ServerBase, SBID_FileTransferA, msg->getSeqIdx(), &fta);

        if (!rv)
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void MsgServerConnect::PrcServerAddressA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileServerA a;

        const std::list<MBCAF::Proto::IPAddress> & addrs = M_Only(TransferManager)->getServerList();
        std::list<MBCAF::Proto::IPAddress>::const_iterator i, iend = addrs.end();
        for(i = addrs.begin(); i != iend; ++i)
        {
            MBCAF::Proto::IPAddress * addr = a.add_ip_addr_list();
            *addr = *i;
            Mlog("Upload file_client_conn addr info, ip=%s, port=%d", addr->ip().c_str(), addr->port());
        }

        send(MTID_ServerBase, SBID_FileServerA, msg->getSeqIdx(), &a);
    }
    //-----------------------------------------------------------------------
}