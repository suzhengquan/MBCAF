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

#include "MdfTransferTask.h"
#include "MdfStrUtil.h"
#include "MBCAF.Proto.pb.h"
#include <uuid/uuid.h>
//#include <pthread.h>

using namespace MBCAF::Proto;
namespace Mdf
{
    class OfflineFileHeader
    {
    public:
        OfflineFileHeader()
        {
            mTID[0] = '\0';
            mFromID[0] = '\0';
            mToID[0] = '\0';
            mCreateTime[0] = '\0';
            mFileName[0] = '\0';
            mSize[0] = '\0';
        }

        void setID(String & _task_id)
        {
            strncpy(mTID, _task_id.c_str(), 128 < _task_id.length() ? 128 : _task_id.length());
        }

        String getID() const
        {
            return mTID;
        }

        void setFromID(Mui32 id)
        {
            sprintf(mFromID, "%u", id);
        }

        void setToID(Mui32 id)
        {
            sprintf(mToID, "%u", id);
        }

        void setCreateTime(time_t t)
        {
            sprintf(mCreateTime, "%ld", t);
        }

        void setFileName(const char* p)
        {
            sprintf(mFileName, p, 512 < strlen(p) ? 512 : strlen(p));
        }

        void setFileSize(Mui32 size)
        {
            sprintf(mSize, "%u", size);
        }

        Mui32 getFromID() const
        {
            return strtoi(String(mFromID));
        }

        Mui32 getToID() const
        {
            return strtoi(String(mToID));
        }

        Mui32 getCreateTime() const
        {
            return strtoi(String(mCreateTime));
        }

        String getFileName() const
        {
            return mFileName;
        }

        Mui32 getFileSize() const
        {
            return strtoi(String(mSize));
        }

        char mTID[128];
        char mFromID[64];
        char mToID[64];
        char mCreateTime[128];
        char mFileName[512];
        char mSize[64];
    };
    //-----------------------------------------------------------------------
    String GenerateUUID()
    {
        String rv;
        uuid_t uid = { 0 };
        uuid_generate(uid);
        if(!uuid_is_null(uid))
        {
            char str_uuid[64] = { 0 };
            uuid_unparse(uid, str_uuid);
            rv = str_uuid;
        }
        return rv;
    }
    //-----------------------------------------------------------------------
    const char * getOfflinePath()
    {
        static const char * gSavePath = NULL;

        if(gSavePath == NULL)
        {
            static char tmpstr[BUFSIZ];
            char work_path[BUFSIZ];
            if (!getcwd(work_path, BUFSIZ))
            {
                Mlog("getcwd %s failed", work_path);
            }
            else
            {
                snprintf(tmpstr, BUFSIZ, "%s/offline_file", work_path);
            }

            Mlog("save offline files to %s", tmpstr);

            int ret = mkdir(tmpstr, 0755);
            if ((ret != 0) && (errno != EEXIST))
            {
                Mlog("!!!Mkdir %s failed to save offline files", tmpstr);
            }

            gSavePath = tmpstr;
        }
        return gSavePath;
    }
    //-----------------------------------------------------------------------
    static FILE * OpenByRead(const String & tid, Mui32 uid)
    {
        FILE * fp = NULL;
        if (tid.length() >= 2)
        {
            char save_path[BUFSIZ];
            snprintf(save_path, BUFSIZ, "%s/%s/%s", getOfflinePath(), tid.substr(0, 2).c_str(), tid.c_str());
            fp = fopen(save_path, "rb");
            if (!fp)
            {
                Mlog("Open file %s for read failed", save_path);
            }
        }
        return fp;
    }
    //-----------------------------------------------------------------------
    static FILE * OpenByWrite(const String & tid, Mui32 uid)
    {
        FILE * fp = NULL;
        if (tid.length() >= 2)
        {
            char save_path[BUFSIZ];

            snprintf(save_path, BUFSIZ, "%s/%s", getOfflinePath(), tid.substr(0, 2).c_str());
            int ret = mkdir(save_path, 0755);
            if ((ret != 0) && (errno != EEXIST))
            {
                Mlog("Mkdir failed for path: %s", save_path);
            }
            else
            {
                // sample g_path/to_id_url/tid
                strncat(save_path, "/", BUFSIZ);
                strncat(save_path, tid.c_str(), BUFSIZ);

                fp = fopen(save_path, "ab+");
                if (!fp)
                {
                    Mlog("Open file for write failed");
                }
            }
        }
        return fp;
    }
    //----------------------------------------------------------------------------
    TransferTask::TransferTask(const String & tid, Mui32 fromid,
        Mui32 toid, const String & filename, Mui32 size) :
        mID(tid),
        mFromID(fromid),
        mToID(toid),
        mFileName(filename),
        mFileSize(size),
        mState(TST_Ready)
    {
        mCreateTime = time(NULL);

        mFromConnect = NULL;
        mToConnect = NULL;
    }
    //-----------------------------------------------------------------------
    void TransferTask::setConnect(Mui32 uid, ClientIO * conn)
    {
        if (mFromID == uid)
        {
            mFromConnect = conn;
        }
        else if (mToID == uid)
        {
            mToConnect = conn;
        }
    }
    //-----------------------------------------------------------------------
    bool TransferTask::processPull(Mui32 uid, int file_role)
    {
        return false;
    }
    //-----------------------------------------------------------------------
    int TransferTask::processRecv(Mui32 uid, Mui32 offset, Mui32 data_size, const char * in)
    { 
        return -1; 
    }
    //-----------------------------------------------------------------------
    int TransferTask::processSend(Mui32 uid, Mui32 offset, Mui32 data_size, String* data)
    { 
        return -1; 
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // OnlineTransferTask
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    OnlineTransferTask::OnlineTransferTask(const String & tid, Mui32 fromid, Mui32 toid,
        const String & filename, Mui32 size) :
        TransferTask(tid, fromid, toid, filename, size)
    {
        mSeqNum = 0;
    }
    //-----------------------------------------------------------------------
    OnlineTransferTask::~OnlineTransferTask()
    {
    }
    //-----------------------------------------------------------------------
    void OnlineTransferTask::setSeqIdx(Mui32 seq)
    {
        mSeqNum = seq;
    }
    //-----------------------------------------------------------------------
    Mui32 OnlineTransferTask::getSeqIdx() const
    {
        return mSeqNum;
    }
    //-----------------------------------------------------------------------
    Mui32 OnlineTransferTask::getMode() const
    {
        return MBCAF::Proto::TFT_Online;
    }
    //-----------------------------------------------------------------------
    bool OnlineTransferTask::processPull(Mui32 uid, int file_role)
    {
        bool rv = false;

        do
        {
            if(file_role == CTR_OnlineSender)
            {
                if(mFromID == uid)
                {
                    rv = true;
                }
            }
            else if(file_role == CTR_OnlineRecver)
            {
                if(mToID == uid)
                {
                    rv = true;
                }
            }

            if (!rv)
            {
                Mlog("Check error! uid=%d, file_role=%d", uid, file_role);
                break;
            }

            if (mState != TST_Ready && mState != TST_WaitingSender && mState != TST_WaitingReceiver) 
            {
                Mlog("Invalid state, valid state is TST_Ready or TST_WaitingSender or TST_WaitingReceiver, but state is %d", mState);
                break;
            }

            if (mState == TST_Ready)
            {
                if (file_role == CTR_OnlineSender)
                {
                    mState = TST_WaitingReceiver;
                }
                else
                {
                    mState = TST_WaitingSender;
                }
            }
            else
            {
                if (mState == TST_WaitingReceiver)
                {
                    if (file_role != CTR_OnlineRecver)
                    {
                        Mlog("Invalid user, uid = %d, but mToID = %d", uid, mToID);
                        break;
                    }
                }
                else if (mState == TST_WaitingSender)
                {
                    if (file_role != CTR_OnlineSender)
                    {
                        Mlog("Invalid user, uid = %d, but mToID = %d", uid, mToID);
                        break;
                    }
                }
                mState = TST_TransferReady;
            }
            mCreateTime = time(NULL);
            rv = true;
        } while (0);

        return rv;
    }
    //-----------------------------------------------------------------------
    int OnlineTransferTask::processRecv(Mui32 uid, Mui32 offset, Mui32 data_size, const char * data)
    {
        int rv = -1;

        do
        {
            if (mFromID != uid)
            {
                Mlog("Check error! uid=%d, mFromID=%d, mToID", uid, mFromID, mToID);
                break;
            }
            if (mState != TST_TransferReady && mState != TST_Transfering) 
            {
                Mlog("Check mState! uid=%d, state=%d, but state need TST_TransferReady or TST_Transfering", uid, mState);
                break;
            }
            if (mState == TST_TransferReady)
            {
                mState = TST_Transfering;
            }

            mCreateTime = time(NULL);
            rv = 0;

        } while (0);
        return rv;
    }
    //-----------------------------------------------------------------------
    int OnlineTransferTask::processSend(Mui32 uid, Mui32 offset, 
        Mui32 data_size, String * data)
    {
        int rv = -1;
        do
        {
            if (mState != TST_TransferReady && mState != TST_Transfering)
            {
                Mlog("Check mState! uid=%d, state=%d, but state need TST_TransferReady or TST_Transfering", uid, mState);
                break;
            }

            if (mState == TST_TransferReady)
            {
                mState = TST_Transfering;
            }

            mCreateTime = time(NULL);
            rv = 0;
        } while (0);
        return rv;
    }
    //----------------------------------------------------------------------------
    OfflineTransferTask::OfflineTransferTask(const String & tid, Mui32 fromid, Mui32 toid,
        const String & filename, Mui32 size) :
        TransferTask(tid, fromid, toid, filename, size)
    {
        mFile = NULL;
        mCurrentBlock = 0;

        mSengmentCount = SetMaxSegmentSize(size);
    }
    //----------------------------------------------------------------------------
    OfflineTransferTask::~OfflineTransferTask()
    {
        if (mFile)
        {
            fclose(mFile);
            mFile = NULL;
        }
    }
    //----------------------------------------------------------------------------
    int OfflineTransferTask::getNextBlockSize()
    {
        int block_size = SEGMENT_SIZE;
        if (mCurrentBlock + 1 == mSengmentCount)
        {
            block_size = mFileSize - mCurrentBlock * SEGMENT_SIZE;
        }
        return block_size;
    }
    //----------------------------------------------------------------------------
    OfflineTransferTask * OfflineTransferTask::LoadFromDisk(const String & tid, Mui32 uid) 
    {
        OfflineTransferTask * offline = NULL;

        FILE * fp = OpenByRead(tid, uid);
        if (fp)
        {
            OfflineFileHeader header;
            size_t size = fread(&header, 1, sizeof(header), fp);
            if (size == sizeof(header))
            {
                fseek(fp, 0L, SEEK_END);
                size_t filesize = static_cast<size_t>(ftell(fp)) - size;
                if (filesize == header.getFileSize())
                {
                    offline = new OfflineTransferTask(header.getID(),
                        header.getFromID(),
                        header.getToID(),
                        header.getFileName(),
                        header.getFileSize());
                    if (offline)
                    {
                        offline->setState(TST_WaitingDownload);
                    }
                }
                else
                {
                    Mlog("Offile file size by tid=%s, uid=%u, header_file_size=%u, disk_file_size=%u", tid.c_str(), uid, header.getFileSize(), filesize);
                }
            }
            else
            {
                Mlog("Read header error by tid=%s, uid=%u", tid.c_str(), uid);
            }
            fclose(fp);
        }
        return offline;
    }
    //-----------------------------------------------------------------------
    int OfflineTransferTask::SetMaxSegmentSize(Mui32 size)
    {
        int seg_size = size / SEGMENT_SIZE;
        if (mFileSize % SEGMENT_SIZE != 0)
        {
            seg_size = size / SEGMENT_SIZE + 1;
        }
        return seg_size;
    }
    //-----------------------------------------------------------------------
    Mui32 OfflineTransferTask::getMode() const
    {
        return MBCAF::Proto::TFT_Offline;
    }
    //-----------------------------------------------------------------------
    bool OfflineTransferTask::processPull(Mui32 uid, int file_role)
    {
        bool rv = false;
        do
        {
            if (file_role == CTR_OfflineSender)
            {
                if (mFromID == uid)
                {
                    rv = true;
                }
            }
            else if (file_role == CTR_OfflineRecver)
            {
                if (mToID == uid)
                {
                    rv = true;
                }
            }

            if(!rv)
            {
                Mlog("Check error! uid=%d, file_role=%d", uid, file_role);
                break;
            }

            if(mState != TST_Ready && mState != TST_UploadDone && mState != TST_WaitingDownload)
            {

                Mlog("Invalid state, valid state is TST_Ready or TST_UploadDone, but state is %d", mState);
                break;
            }

            if(mState == TST_Ready)
            {
                if(CTR_OfflineSender == file_role)
                {
                    mState = TST_WaitingUpload;
                }
                else
                {
                    Mlog("Offline upload: file_role is CTR_OfflineSender but file_role = %d", file_role);
                    break;
                }
            }
            else
            {
                if (file_role == CTR_OfflineRecver)
                {
                    mState = TST_WaitingDownload;
                }
                else
                {
                    Mlog("Offline upload: file_role is CTR_OfflineRecver but file_role = %d", file_role);
                    break;
                }
            }
            mCreateTime = time(NULL);
            rv = true;
        } while (0);
        return rv;
    }
    //-----------------------------------------------------------------------
    int OfflineTransferTask::processRecv(Mui32 uid, Mui32 offset, Mui32 data_size, const char * data)
    {
        int rv = -1;
        do
        {
            if (mFromID != uid)
            {
                Mlog("rsp uid=%d, but sender_id is %d", uid, mFromID);
                break;
            }

            if (mState != TST_WaitingUpload && mState != TST_Uploading) 
            {
                Mlog("state=%d error, need TST_WaitingUpload or TST_Uploading", mState);
                break;
            }

            // 检查offset是否有效
            if (offset != mCurrentBlock * SEGMENT_SIZE)
            {
                break;
            }

            data_size = getNextBlockSize();
            Mlog("Ready recv data, offset=%d, data_size=%d, segment_size=%d", offset, data_size, mSengmentCount);

            if (mState == TST_WaitingUpload)
            {
                if (mFile == NULL)
                {
                    mFile = OpenByWrite(mID, mToID);
                    if (mFile == NULL)
                    {
                        break;
                    }
                }

                OfflineFileHeader header;
                memset(&header, 0, sizeof(header));
                header.setCreateTime(time(NULL));
                header.setID(mID);
                header.setFromID(mFromID);
                header.setToID(mToID);
                header.setFileName("");
                header.setFileSize(mFileSize);
                fwrite(&header, 1, sizeof(header), mFile);
                fflush(mFile);

                mState = TST_Uploading;
            }

            if (mFile == NULL)
            {
                break;
            }

            fwrite(data, 1, data_size, mFile);
            fflush(mFile);

            ++mCurrentBlock;
            mCreateTime = time(NULL);

            if (mCurrentBlock == mSengmentCount)
            {
                mState = TST_UploadDone;
                fclose(mFile);
                mFile = NULL;
                rv = 1;
            }
            else
            {
                rv = 0;
            }
        } while (0);

        return rv;
    }
    //-----------------------------------------------------------------------
    int OfflineTransferTask::processSend(Mui32 uid, Mui32 offset,
        Mui32 data_size, String * data) 
    {
        int rv = -1;
        Mlog("Recv pull file request: uid=%d, offset=%d, data_size=%d", uid, offset, data_size);
        do
        {
            if (mState != TST_WaitingDownload && mState != TST_Downloading) 
            {
                Mlog("state=%d error, need TST_WaitingDownload or TST_Downloading", mState);
                break;
            }

            if (mState == TST_WaitingDownload)
            {
                if (mCurrentBlock != 0)
                    mCurrentBlock = 0;

                if (mFile != NULL)
                {
                    fclose(mFile);
                    mFile = NULL;
                }

                mFile = OpenByRead(mID, uid);
                if (mFile == NULL)
                {
                    break;
                }

                OfflineFileHeader header;
                size_t size = fread(&header, 1, sizeof(header), mFile); // read header
                if (sizeof(header) != size)
                {
                    Mlog("read file head failed.");
                    fclose(mFile);
                    mFile = NULL;
                    break;

                }

                mState = TST_Downloading;
            }
            else
            {
                if (mFile == NULL)
                {
                    break;
                }
            }

            if (offset != mCurrentBlock * SEGMENT_SIZE)
            {
                Mlog("Recv offset error, offser=%d, transfered_offset=%d", offset, mCurrentBlock*SEGMENT_SIZE);
                break;
            }

            data_size = getNextBlockSize();

            Mlog("Ready send data, offset=%d, data_size=%d", offset, data_size);

            char * tmpbuf = (char *)malloc(data_size);
            if (NULL == tmpbuf)
            {
                Mlog("alloc mem failed.");
                break;
            }
            memset(tmpbuf, 0, data_size);

            size_t size = fread(tmpbuf, 1, data_size, mFile);
            if (size != data_size)
            {
                Mlog("Read size error, data_size=%d, but read_size=%d", data_size, size);
                free(tmpbuf);
                break;
            }

            data->append(tmpbuf, data_size);
            free(tmpbuf);

            mCurrentBlock++;

            mCreateTime = time(NULL);
            if (mCurrentBlock == mSengmentCount)
            {
                Mlog("pull req end.");
                mState = TST_UploadDone;
                fclose(mFile);
                mFile = NULL;
                rv = 1;
            }
            else
            {
                rv = 0;
            }
        } while (0);
        return rv;
    }
    //-----------------------------------------------------------------------
}
