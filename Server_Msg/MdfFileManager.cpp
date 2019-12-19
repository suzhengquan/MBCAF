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

#include "MdfFileManager.h"
#include "MdfServerConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfDatabaseClientConnect.h"
#include "MdfFileClientConnect.h"
#include "MdfUserManager.h"
#include "MBCAF.HubServer.pb.h"
#include "MBCAF.ServerBase.pb.h"
#include "MBCAF.MsgServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    M_SingletonImpl(FileManager);
    //-----------------------------------------------------------------------
    void FileManager::FileQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileQ proto;
        if(!msg->toProto(&proto)) 
            return;
        
        Mui32 fromid = conn->getUserID();
        Mui32 toid = proto.to_user_id();
        String fname = proto.file_name();
        Mui32 fsize = proto.file_size();
        Mui32 tmode = proto.trans_mode();
        Mlog("FileQ, %u->%u, fileName: %s, trans_mode: %u.", fromid, toid, fname.c_str(), tmode);
        
        FileClientConnect * fileconn = FileClientConnect::getRandomConnect();
        if (fileconn)
        {
            HandleExtData attach(conn->getID());
            MBCAF::HubServer::FileTransferQ proto2;
            proto2.set_from_user_id(fromid);
            proto2.set_to_user_id(toid);
            proto2.set_file_name(fname);
            proto2.set_file_size(fsize);
            proto2.set_trans_mode((MBCAF::Proto::TransferFileType)tmode);
            proto2.set_attach_data(attach.getBuffer(), attach.getSize());
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(SBMSG(FileTransferQ));
            remsg.setSeqIdx(msg->getSeqIdx());
            
            if (MBCAF::Proto::TFT_Offline == tmode)
            {
                fileconn->send(&remsg);
            }
            else
            {
                User * tempusr = M_Only(UserManager)->getUser(toid);
                if (tempusr && tempusr->getPCState())
                {
                    fileconn->send(&remsg);
                }
                else
                {                
                    RouteClientConnect * routeconn = RouteClientConnect::getPrimaryConnect();
                    if (routeconn)
                    {
                        MessageExtData extdata(MEDT_ProtoToFile, conn->getID(), remsg.getContentSize(), remsg.getContent());
                        MBCAF::MsgServer::BuddyObjectStateQ proto3;
                        proto3.set_user_id(fromid);
                        proto3.add_user_id_list(toid);
                        proto3.set_attach_data(extdata.getBuffer(), extdata.getSize());
                        MdfMessage remsg2;
                        remsg2.setProto(&proto3);
                        remsg2.setCommandID(MSMSG(BuddyObjectStateQ));
                        remsg2.setSeqIdx(msg->getSeqIdx());

                        routeconn->send(&remsg2);
                    }
                }
            }
        }
        else
        {
            Mlog("FileQ, no file server.   ");
            MBCAF::HubServer::FileA proto2;
            proto2.set_result_code(1);
            proto2.set_from_user_id(fromid);
            proto2.set_to_user_id(toid);
            proto2.set_file_name(fname);
            proto2.set_task_id("");
            proto2.set_trans_mode((MBCAF::Proto::TransferFileType)tmode);
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(HSMSG(FileA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void FileManager::OfflineFileQ(ServerConnect * conn, MdfMessage * msg)
    {
        Mui32 userid = conn->getUserID();
        Mlog("OfflineFileQ, req_id=%u   ", userid);
        
        HandleExtData extdata(conn->getID());
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn) 
        {
            MBCAF::HubServer::FileExistOfflineQ proto;
            if(!msg->toProto(&proto)) 
                return;
            proto.set_user_id(userid);
            proto.set_attach_data(extdata.getBuffer(), extdata.getSize());
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        else
        {
            Mlog("warning no DB connection available ");
            MBCAF::HubServer::FileExistOfflineA proto;
            proto.set_user_id(userid);
            MdfMessage remsg;
            remsg.setProto(&proto);
            remsg.setCommandID(HSMSG(OfflineFileA));
            remsg.setSeqIdx(msg->getSeqIdx());
            conn->send(&remsg);
        }
    }
    //-----------------------------------------------------------------------
    void FileManager::AddOfflineFileQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileAddOfflineQ proto;
        if(!msg->toProto(&proto)) 
            return;
        
        Mui32 fromid = conn->getUserID();
        Mui32 toid = proto.to_user_id();
        String taskid = proto.task_id();
        String fname = proto.file_name();
        Mui32 fsize = proto.file_size();
        Mlog("AddOfflineFileQ, %u->%u, task_id: %s, file_name: %s, size: %u  ",
            fromid, toid, taskid.c_str(), fname.c_str(), fsize);
        
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn) 
        {
            proto.set_from_user_id(fromid);
            msg->setProto(&proto);
            dbconn->send(msg);
        }
        
        FileClientConnect * fileconn = FileClientConnect::getRandomConnect();
        if (fileconn)
        {
            const list<MBCAF::Proto::IPAddress> & addrlist = fileconn->GetFileServerIPList();
            
            MBCAF::HubServer::FileNotify proto2;
            proto2.set_from_user_id(fromid);
            proto2.set_to_user_id(toid);
            proto2.set_file_name(fname);
            proto2.set_file_size(fsize);
            proto2.set_task_id(taskid);
            proto2.set_trans_mode(MBCAF::Proto::TFT_Offline);
            proto2.set_offline_ready(1);
            list<MBCAF::Proto::IPAddress>::const_iterator it, itend = addrlist.end();
            for (it = addrlist.begin(); it != itend; ++it)
            {
                MBCAF::Proto::IPAddress * ip_addr = proto2.add_ip_addr_list();
                ip_addr->set_ip((*it).ip());
                ip_addr->set_port((*it).port());
            }
            MdfMessage remsg;
            remsg.setProto(&proto2);
            remsg.setCommandID(HSMSG(FileNotify));
            
            User * tempusr = M_Only(UserManager)->getUser(toid);
            if (tempusr)
            {
                tempusr->broadcastToPC(&remsg);
            }
            RouteClientConnect * conn = RouteClientConnect::getPrimaryConnect();
            if (conn) 
            {
                conn->send(&remsg);
            }
        }
    }
    //-----------------------------------------------------------------------
    void FileManager::DeleteOfflineFileQ(ServerConnect * conn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileDeleteOfflineQ proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 fromid = proto.from_user_id();
        Mui32 toid = proto.to_user_id();
        String taskid = proto.task_id();
        Mlog("DeleteOfflineFileQ, %u->%u, task_id=%s ", fromid, toid, taskid.c_str());
        
        DataBaseClientConnect * dbconn = DataBaseClientConnect::getPrimaryConnect();
        if (dbconn) 
        {
            proto.set_from_user_id(fromid);
            msg->setProto(&proto);
            dbconn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void FileManager::prcOfflineFileA(MdfMessage * msg)
    {
        MBCAF::HubServer::FileExistOfflineA proto;
        if(!msg->toProto(&proto)) 
            return;

        Mui32 userid = proto.user_id();
        Mui32 filecnt = proto.offline_file_list_size();
        HandleExtData attach((Mui8*)proto.attach_data().c_str(), proto.attach_data().length());
        Mlog("prcOfflineFileA, req_id=%u, file_cnt=%u ", userid, filecnt);
        
        ServerConnect * conn = M_Only(UserManager)->getMsgConnect(userid, attach.getHandle());
        
        if (conn) 
        {
            FileClientConnect * fileconn = FileClientConnect::getRandomConnect();
            if (fileconn)
            {
                const list<MBCAF::Proto::IPAddress> & iplist = fileconn->GetFileServerIPList();
                for(list<MBCAF::Proto::IPAddress>::const_iterator it = iplist.begin(); it != iplist.end(); ++it)
                {
                    MBCAF::Proto::IPAddress * ip_addr = proto.add_ip_addr_list();
                    ip_addr->set_ip((*it).ip());
                    ip_addr->set_port((*it).port());
                }
            }
            else
            {
                Mlog("prcOfflineFileA, no file server. ");
            }

            msg->setProto(&proto);
            conn->send(msg);
        }
    }
    //-----------------------------------------------------------------------
    void FileManager::prcFileNotify(MdfMessage * msg)
    {
        MBCAF::HubServer::FileNotify proto;
        if(!msg->toProto(&proto))
            return;

        Mui32 fromid = proto.from_user_id();
        Mui32 userid = proto.to_user_id();
        String fname = proto.file_name();
        String taskid = proto.task_id();
        Mui32 tmode = proto.trans_mode();
        Mui32 ready = proto.offline_ready();
        Mlog("prcFileNotify, from_id: %u, to_id: %u, file_name: %s, task_id: %s, trans_mode: %u,\
            offline_ready: %u. ", fromid, userid, fname.c_str(), taskid.c_str(), tmode, ready);
        User * tempusr = M_Only(UserManager)->getUser(userid);
        if(tempusr)
        {
            tempusr->broadcast(msg);
        }
    }
    //-----------------------------------------------------------------------
}