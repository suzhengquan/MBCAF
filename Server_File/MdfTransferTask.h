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

#ifndef _MDF_TRANSFER_TASK_H_
#define _MDF_TRANSFER_TASK_H_

#include "MdfPreInclude.h"
#include "MdfStrUtil.h"

namespace Mdf
{
    class ClientIO;
    /**
    @version 0.9.1
    */
    enum TaskStateType
    {
        TST_Unknow            = 0,

        TST_Ready            = 1,

        TST_WaitingSender    = 2,
        TST_WaitingReceiver = 3,
        TST_TransferReady    = 4,
        TST_Transfering        = 5,
        TST_TransferDone    = 6,

        TST_WaitingUpload    = 7,
        TST_Uploading        = 8,
        TST_UploadDone        = 9,

        TST_WaitingDownload = 10,
        TST_Downloading        = 11,
        TST_DownloadDone    = 12,

        TST_Error            = 13
    };
    /**
    @version 0.9.1
    */
    class TransferTask
    {
    public:
        TransferTask(const String & id, Mui32 fromuser, Mui32 touser, const String & file, Mui32 size);
        virtual ~TransferTask() {}

        /**
        @version 0.9.1
        */
        inline const String & getID() const { return mID; }

        /**
        @version 0.9.1
        */
        inline Mui32 getFromID() const { return mFromID; }

        /**
        @version 0.9.1
        */
        inline Mui32 getToID() const { return  mToID; }

        /**
        @version 0.9.1
        */
        ClientIO * getFromConnect() const
        {
            return mFromConnect;
        }

        /**
        @version 0.9.1
        */
        ClientIO * getToConnect() const
        {
            return mToConnect;
        }

        /**
        @version 0.9.1
        */
        void setConnect(Mui32 id, ClientIO * conn);

        /**
        @version 0.9.1
        */
        virtual Mui32 getMode() const = 0;

        /**
        @version 0.9.1
        */
        inline const String & getFileName() const { return mFileName; }

        /**
        @version 0.9.1
        */
        inline Mui32 getFileSize() const { return mFileSize; }

        /**
        @version 0.9.1
        */
        inline time_t getCreateTime() const { return mCreateTime; }

        /**
        @version 0.9.1
        */
        inline void setState(int state) { mState = state; }

        /**
        @version 0.9.1
        */
        inline int getState() const { return mState; }

        /**
        @version 0.9.1
        */
        virtual bool processPull(Mui32 id, int op);

        /**
        @version 0.9.1
        */
        virtual int processRecv(Mui32 id, Mui32 offset, Mui32 size, const char * in);

        /**
        @version 0.9.1
        */
        virtual int processSend(Mui32 id, Mui32 offset, Mui32 size, String * out);
    protected:
        String mID;
        Mui32 mFromID;
        Mui32 mToID;
        ClientIO * mFromConnect;
        ClientIO * mToConnect;
        String mFileName;
        Mui32 mFileSize;
        time_t mCreateTime;
        int mState;
    };

    typedef map<String, TransferTask *> TaskList;
    typedef map<ClientIO *, TransferTask *> ConnectTaskList;

    class OnlineTransferTask : public TransferTask
    {
    public:
        OnlineTransferTask(const String & tid, Mui32 fromid, Mui32 toid,
            const String & filename, Mui32 size);

        virtual ~OnlineTransferTask();

        void setSeqIdx(Mui32 seq);

        Mui32 getSeqIdx() const;

        /// @copydetails TransferTask::getMode
        virtual Mui32 getMode() const;

        /// @copydetails TransferTask::processPull
        virtual bool processPull(Mui32 id, int op);

        /// @copydetails TransferTask::processRecv
        virtual int processRecv(Mui32 id, Mui32 offset,  Mui32 data_size, const char * in);

        /// @copydetails TransferTask::processSend
        virtual int processSend(Mui32 id, Mui32 offset, Mui32 data_size, String * out);
    private:
        Mui32 mSeqNum;
    };

    #define SEGMENT_SIZE 32768

    class OfflineTransferTask : public TransferTask
    {
    public:
        OfflineTransferTask(const String & tid, Mui32 fromid, Mui32 toid,
            const String & filename, Mui32 size);

        virtual ~OfflineTransferTask();

        int getNextBlockSize();

        inline Mui32 getNextBlockOffset() const
        {
            return SEGMENT_SIZE * mCurrentBlock;
        }

        inline int getSengmentCount() const
        {
            return mSengmentCount;
        }

        /// @copydetails TransferTask::getMode
        virtual Mui32 getMode() const;

        /// @copydetails TransferTask::processPull
        virtual bool processPull(Mui32 id, int op);

        /// @copydetails TransferTask::processRecv
        virtual int processRecv(Mui32 id, Mui32 offset, Mui32 data_size, const char * in);

        /// @copydetails TransferTask::processSend
        virtual int processSend(Mui32 id, Mui32 offset, Mui32 data_size, String * out);

        static OfflineTransferTask * LoadFromDisk(const String & getID, Mui32 user_id);
    private:
        int SetMaxSegmentSize(Mui32 size);
    private:
        FILE * mFile;
        int mCurrentBlock;
        int mSengmentCount;
    };

    String GenerateUUID();
}
#endif
