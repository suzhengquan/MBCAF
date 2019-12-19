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

#include "MdfModel_File.h"
#include "MdfServerConnect.h"
#include "MdfDatabaseManager.h"
#include "MBCAF.HubServer.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    void OfflineFileExistA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileExistOfflineQ proto;
        MBCAF::HubServer::FileExistOfflineA protoA;
        if (msg->toProto(&proto))
        {
            MdfMessage * remsg = new MdfMessage;

            uint32_t userid = proto.user_id();
            Model_File* pModel = M_Only(Model_File);
            list<MBCAF::Proto::OfflineFileInfo> lsOffline;
            pModel->getOfflineFile(userid, lsOffline);
            protoA.set_user_id(userid);
            for (list<MBCAF::Proto::OfflineFileInfo>::iterator it = lsOffline.begin();
                it != lsOffline.end(); ++it)
            {
                MBCAF::Proto::OfflineFileInfo* pInfo = protoA.add_offline_file_list();
                pInfo->set_from_user_id(it->from_user_id());
                pInfo->set_task_id(it->task_id());
                pInfo->set_file_name(it->file_name());
                pInfo->set_file_size(it->file_size());
            }

            Mlog("userId=%u, count=%u", userid, protoA.offline_file_list_size());

            protoA.set_attach_data(proto.attach_data());
            remsg->setProto(&protoA);
            remsg->setSeqIdx(msg->getSeqIdx());
            remsg->setCommandID(HSMSG(OfflineFileA));
            M_Only(DataSyncManager)->response(qconn, remsg);
        }
        else
        {
            Mlog("parse pb failed");
        }
    }
    //-----------------------------------------------------------------------
    void AddOfflineFileA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileAddOfflineQ proto;
        if (msg->toProto(&proto))
        {
            uint32_t fromid = proto.from_user_id();
            uint32_t toid = proto.to_user_id();
            String strTaskId = proto.task_id();
            String strFileName = proto.file_name();
            uint32_t nFileSize = proto.file_size();
            Model_File* pModel = Model_File::getInstance();
            pModel->addOfflineFile(fromid, toid, strTaskId, strFileName, nFileSize);
            Mlog("fromId=%u, toId=%u, taskId=%s, fileName=%s, fileSize=%u", fromid, toid, strTaskId.c_str(), strFileName.c_str(), nFileSize);
        }
    }
    //-----------------------------------------------------------------------
    void DeleteOfflineFileA(ServerConnect * qconn, MdfMessage * msg)
    {
        MBCAF::HubServer::FileDeleteOfflineQ proto;
        if (msg->toProto(&proto))
        {
            uint32_t fromid = proto.from_user_id();
            uint32_t toid = proto.to_user_id();
            String strTaskId = proto.task_id();
            Model_File* pModel = Model_File::getInstance();
            pModel->deleteOfflineFile(fromid, toid, strTaskId);
            Mlog("fromId=%u, toId=%u, taskId=%s", fromid, toid, strTaskId.c_str());
        }
    }
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_File);
    //-----------------------------------------------------------------------
    Model_File::Model_File()
    {
    }
    //-----------------------------------------------------------------------
    Model_File::~Model_File()
    {
    }
    //-----------------------------------------------------------------------
    void Model_File::getOfflineFile(uint32_t userid, list<MBCAF::Proto::OfflineFileInfo> & lsOffline)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select * from IMTransmitFile where toId=" + itostr(userid) + " and state=0 order by created";
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    String temp;
                    Mi32 temp2;
                    MBCAF::Proto::OfflineFileInfo offlineFile;
                    resSet->getValue("fromId", temp2);
                    offlineFile.set_from_user_id(temp2);
                    resSet->getValue("taskId", temp);
                    offlineFile.set_task_id(temp);
                    resSet->getValue("fileName", temp);
                    offlineFile.set_file_name(temp.c_str());
                    resSet->getValue("size", temp2);
                    offlineFile.set_file_size(temp2);
                    lsOffline.push_back(offlineFile);
                }
                delete resSet;
            }
            else
            {
                Mlog("no result for:%s", strSql.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //-----------------------------------------------------------------------
    void Model_File::addOfflineFile(uint32_t fromid, uint32_t toid, String & taskId, String & fileName, uint32_t fileSize)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "insert into IMTransmitFile (`fromId`,`toId`,`fileName`,`size`,`taskId`,`state`,`created`,`updated`) values(?,?,?,?,?,?,?,?)";

            PrepareExec * pStmt = new PrepareExec();
            if (pStmt->prepare(dbConn->getConnect(), strSql))
            {
                uint32_t state = 0;
                uint32_t nCreated = (uint32_t)time(NULL);

                uint32_t index = 0;
                pStmt->setParam(index++, fromid);
                pStmt->setParam(index++, toid);
                pStmt->setParam(index++, fileName);
                pStmt->setParam(index++, fileSize);
                pStmt->setParam(index++, taskId);
                pStmt->setParam(index++, state);
                pStmt->setParam(index++, nCreated);
                pStmt->setParam(index++, nCreated);

                bool bRet = pStmt->exec();

                if (!bRet)
                {
                    Mlog("insert message failed: %s", strSql.c_str());
                }
            }
            delete pStmt;
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
    }
    //-----------------------------------------------------------------------
    void Model_File::deleteOfflineFile(uint32_t fromid, uint32_t toid, String & taskId)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
        if (dbConn)
        {
            String strSql = "delete from IMTransmitFile where  fromId=" + itostr(fromid) + " and toId=" + itostr(toid) + " and taskId='" + taskId + "'";
            if (dbConn->exec(strSql.c_str()))
            {
                Mlog("delete offline file success.%d->%d:%s", fromid, toid, taskId.c_str());
            }
            else
            {
                Mlog("delete offline file failed.%d->%d:%s", fromid, toid, taskId.c_str());
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsmaster");
        }
    }
    //-----------------------------------------------------------------------
}