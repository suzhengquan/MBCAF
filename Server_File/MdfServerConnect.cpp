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

#include "MdfServerConnect.h"
#include "MdfTransferTaskManager.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.HubServer.pb.h"

#define SOCKET_BUF_SIZE (256 * 1024)

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect(ACE_Reactor * tor) :
        ServerIO(tor)
        mUserCheck(false),
        mUserID(0),
        mTask(NULL) 
    {
    }
    //-----------------------------------------------------------------------
    ServerConnect::~ServerConnect() 
    {
    }
    //-----------------------------------------------------------------------
    ServerIO * ServerConnect::createInstance() const
    {
        return new ServerConnect();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::resetTask()
    {
        mUserID = 0;
        mTask = NULL;
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        setTimer(true, 0, 1000);
        setSendSize(SOCKET_BUF_SIZE);
        setRecvSize(SOCKET_BUF_SIZE);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        if (mTask)
        {
            if (mTask->getMode() == TFT_Online)
            {
                mTask->setState(TST_Unknow);
            }
            else
            {
                if (mTask->getState() >= TST_UploadDone)
                {
                    mTask->setState(TST_WaitingDownload);
                }
            }
            mTask->setConnect(mUserID, NULL);
            M_Only(TransferManager)->destroyConnectCloseTask(mTask->getID());
            mTask = NULL;
        }
        mUserCheck = false;
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS curr_tick)
    {
        if (mTask && mTask->getMode() == TFT_Online)
        {
            if (mTask->getState() == TST_Unknow)
            {
                Mlog("Close another online conn, user_id=%d", mUserID);
                stop();
                return;
            }
        }
        if (curr_tick > mReceiveMark + M_Client_Timeout)
        {
            Mlog("client timeout, user_id=%u", mUserID);
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case SBID_Heartbeat:
            prcHeartbeat(temp);
            break;
        case HSID_LoginQ:
            PrcClientFileLoginA(temp);
            break;
        case HSID_FileState:
            PrcFileState(temp);
            break;
        case HSID_PullQ:
            PrcClientFilePullA(temp);
            break;
        case HSID_PullA:
            PrcClientFilePullQ(temp);
            break;
        default:
            Mlog("no such cmd id: %u", temp->getCommandID());
            break;
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcHeartbeat(MdfMessage * msg)
    {
        send(msg);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::PrcClientFileLoginA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileLoginQ flq;
        if (!msg->toProto(&flq))
            return;

        Mui32 user_id = flq.user_id();
        String tid = flq.getID();
        MBCAF::Proto::ClientTransferRole mode = flq.file_role();

        Mlog("Client login, user_id=%d, tid=%s, file_role=%d", user_id, tid.c_str(), mode);

        TransferTask * task = NULL;

        bool rv = false;
        do
        {
            task = M_Only(TransferManager)->getTask(tid);
            if (task == NULL)
            {
                if (mode == CTR_OfflineRecver)
                {
                    task = M_Only(TransferManager)->createTask(tid, user_id);
                    if (task == NULL)
                    {
                        Mlog("Find task id failed, user_id=%u, taks_id=%s, mode=%d", user_id, tid.c_str(), mode);
                        break;
                    }
                }
                else
                {
                    Mlog("Can't find tid, user_id=%u, taks_id=%s, mode=%d", user_id, tid.c_str(), mode);
                    break;
                }
            }

            rv = task->processPull(user_id, mode);
            if (!rv)
            {
                break;
            }

            mUserCheck = true;
            mTask = task;
            mUserID = user_id;
            task->setConnect(user_id, this);
            rv = true;

        } while (0);

        MBCAF::HubServer::FileLoginA fla;
        fla.set_result_code(rv ? 0 : 1);
        fla.setID(tid);

        send(MTID_HubServer, HSID_LoginA, msg->getSeqIdx(), &fla);

        if (rv)
        {
            if (task->getMode() == TFT_Online)
            {
                if (task->getState() == TST_TransferReady)
                {
                    ServerIO * conn = mTask->getToConnect();
                    if (conn)
                    {
                        sendNotify(TFS_PeerReady, tid, mTask->getFromID(), conn);
                    }
                    else
                    {
                        Mlog("to_conn is close, close me!!!");
                        stop();
                    }
                }
            }
            else
            {
                if (task->getState() == TST_WaitingUpload)
                {
                    OfflineTransferTask * offline = reinterpret_cast<OfflineTransferTask *>(task);

                    MBCAF::HubServer::FilePullQ fpq;
                    fpq.setID(tid);
                    fpq.set_user_id(user_id);
                    fpq.set_trans_mode(TFT_Offline);
                    fpq.set_offset(0);
                    fpq.set_data_size(offline->getNextBlockSize());

                    send(MTID_HubServer, HSID_PullQ, &fpq);

                    Mlog("Pull Data Req");
                }
            }
        }
        else
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::PrcFileState(MdfMessage * msg)
    {
        if (!mUserCheck || !mTask)
        {
            Mlog("Recv a client_file_state, but auth is false");
            return;
        }

        MBCAF::HubServer::FileTransferState fts;
        if (!msg->toProto(&fts))
            return;

        String tid = fts.task_id();
        Mui32 user_id = fts.user_id();
        Mui32 file_stat = fts.state();

        Mlog("Recv FileState, user_id=%d, tid=%s, file_stat=%d", user_id, tid.c_str(), file_stat);

        bool rv = false;
        do
        {
            if (user_id != mUserID)
            {
                Mlog("Received user_id valid, recv_user_id = %d, task.user_id = %d, mUserID = %d", user_id, mTask->getFromID(), mUserID);
                break;
            }

            if (mTask->getID() != tid)
            {
                Mlog("Received tid valid, recv_task_id = %s, this_task_id = %s", tid.c_str(), mTask->getID().c_str());
                break;
            }

            switch (file_stat)
            {
            case TFS_Cancel:
            case TFS_Done:
            case TFS_Refuse:
            {
                ServerIO * im_conn = user_id == mTask->getFromID() ? mTask->getToConnect() : mTask->getFromConnect();
                if (im_conn)
                {
                    im_conn->send(msg);
                    Mlog("Task %s %d by user_id %d notify %d, erased", tid.c_str(), file_stat, user_id, (user_id == mTask->getFromID() ? user_id : mTask->getFromID()));
                }

                rv = true;
                break;
            }

            default:
                Mlog("Recv valid file_stat: filestat = %d, user_id=%d, tid=%s", file_stat, mUserID, tid.c_str());
                break;
            }
        } while (0);

        stop();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::PrcClientFilePullA(MdfMessage * msg)
    {
        if (!mUserCheck || !mTask)
        {
            Mlog("Recv a client_file_state, but auth is false");
            return;
        }

        MBCAF::HubServer::FilePullQ fpq;
        if (!msg->toProto(&fpq))
            return;

        Mui32 user_id = fpq.user_id();
        String tid = fpq.getID();
        Mui32 mode = fpq.trans_mode();
        Mui32 offset = fpq.offset();
        Mui32 datasize = fpq.data_size();

        Mlog("Recv FilePullFileReq, user_id=%d, tid=%s, file_role=%d, offset=%d, datasize=%d", user_id, tid.c_str(), mode, offset, datasize);

        MBCAF::HubServer::FilePullA fpa;
        fpa.set_result_code(1);
        fpa.setID(tid);
        fpa.set_user_id(user_id);
        fpa.set_offset(offset);
        fpa.set_file_data("");

        int rv = -1;

        do
        {
            if (user_id != mUserID)
            {
                Mlog("Received user_id valid, recv_user_id = %d, task.user_id = %d, mUserID = %d", user_id, mTask->getFromID(), mUserID);
                break;
            }

            if (mTask->getID() != tid)
            {
                Mlog("Received tid valid, recv_task_id = %s, this_task_id = %s", tid.c_str(), mTask->getID().c_str());
                break;
            }

            if (mTask->getToID() != user_id)
            {
                Mlog("user_id equal task.to_user_id, but user_id=%d, task.to_user_id=%d", user_id, mTask->getToID());
                break;
            }

            rv = mTask->processSend(user_id, offset, datasize, fpa.mutable_file_data());

            if (rv == -1)
            {
                break;
            }

            fpa.set_result_code(0);

            if (mTask->getMode() == TFT_Online)
            {
                OnlineTransferTask * online = reinterpret_cast<OnlineTransferTask*>(mTask);
                online->setSeqIdx(msg->getSeqIdx());
                
                ServerIO * conn = user_id == mTask->getFromID() ? mTask->getToConnect() : mTask->getFromConnect();
                if (conn)
                {
                    conn->send(msg);
                }
            }
            else
            {
                this->send(MTID_HubServer, HSID_PullA, msg->getSeqIdx(), &fpa);
                if (rv == 1)
                {
                    sendNotify(TFS_Done, tid, mTask->getFromID(), this);
                }
            }
        } while (0);

        if (rv != 0)
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::PrcClientFilePullQ(MdfMessage * msg)
    {
        if (!mUserCheck || !mTask)
        {
            Mlog("auth is false");
            return;
        }

        MBCAF::HubServer::FilePullA fpa;
        if (!msg->toProto(&fpa))
            return;

        Mui32 user_id = fpa.user_id();
        String tid = fpa.getID();
        Mui32 offset = fpa.offset();
        Mui32 data_size = static_cast<Mui32>(fpa.file_data().length());
        const char * data = fpa.file_data().data();

        Mlog("Recv FilePullFileRsp, tid=%s, user_id=%u, offset=%u, data_size=%d", tid.c_str(), user_id, offset, data_size);

        int rv = -1;
        do
        {

            if (user_id != mUserID)
            {
                Mlog("Received user_id valid, recv_user_id = %d, task.user_id = %d, mUserID = %d", user_id, mTask->getFromID(), mUserID);
                break;
            }
            if (mTask->getID() != tid)
            {
                Mlog("Received tid valid, recv_task_id = %s, this_task_id = %s", tid.c_str(), mTask->getID().c_str());
                break;
            }

            rv = mTask->processRecv(user_id, offset, data_size, data);
            if (rv == -1)
            {
                break;
            }

            if (mTask->getMode() == TFT_Online)
            {
                OnlineTransferTask * online = reinterpret_cast<OnlineTransferTask*>(mTask);
                msg->setSeqIdx(online->getSeqIdx());

                ServerIO * conn = mTask->getToConnect();
                if (conn)
                {
                    conn->send(msg);
                }
            }
            else
            {
                if (rv == 1)
                {
                    sendNotify(TFS_Done, tid, user_id, this);
                }
                else
                {
                    OfflineTransferTask * offline = reinterpret_cast<OfflineTransferTask*>(mTask);

                    MBCAF::HubServer::FilePullQ fpq;
                    fpq.setID(tid);
                    fpq.set_user_id(user_id);
                    fpq.set_trans_mode(static_cast<MBCAF::Proto::TransferFileType>(offline->getMode()));
                    fpq.set_offset(offline->getNextBlockOffset());
                    fpq.set_data_size(offline->getNextBlockSize());

                    this->send(MTID_HubServer, HSID_PullQ, &fpq);
                }
            }

        } while (0);

        if (rv != 0)
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    int ServerConnect::sendNotify(int state, const std::String & tid, Mui32 uid, ServerIO * conn)
    {
        ServerConnect * file_client_conn = reinterpret_cast<ServerConnect *>(conn);

        MBCAF::HubServer::FileTransferState fts;
        fts.set_state(static_cast<TransferFileState>(state));
        fts.set_task_id(tid);
        fts.set_user_id(uid);

        conn->send(MTID_HubServer, HSID_FileState, &fts);

        Mlog("notify to user %d state %d task %s", uid, state, tid.c_str());
        return 0;
    }
    //-----------------------------------------------------------------------
}