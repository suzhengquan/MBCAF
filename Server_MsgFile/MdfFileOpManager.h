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

#ifndef _MDF_FILEOP_MANAGER_H_
#define _MDF_FILEOP_MANAGER_H_

#include "MdfPreInclude.h"
#include "MdfFileOp.h"
#include "MdfThreadManager.h"
#include "MdfConnectManager.h"

namespace Mdf
{
    struct Response;
    class FileEntry;
    class FileOpManager : public ACE_Event_Handler, public ConnectManager::TimerListener
    {
    public:
        /**
        @version 0.9.1
        */
        static bool isExist(const String & path);

        /**
        @version 0.9.1
        */
        static Mi64 getFileSize(const String & path);

        /**
        @version 0.9.1
        */
        bool setInfo(const String & host, const String & disk, int total, int perDirCnt);

        /**
        @version 0.9.1
        */
        Mui64 getFileCount();

        /**
        @version 0.9.1
        */
        String getRelPath();

        /**
        @version 0.9.1
        */
        int uploadFile(String & url, const char * type, const void * data, Mui32 size, char * ext = NULL);

        /**
        @version 0.9.1
        */
        int downloadFile(const String & url, void * buf, Mui32 * size);

        /**
        @version 0.9.1
        */
        int getURLRelPath(const String & url, String & path);

        /**
        @version 0.9.1
        */
        int getURLAbsPath(const String & url, String & path);

        /**
        @version 0.9.1
        */
        void setupPostThread(MCount cnt);

        /**
        @version 0.9.1
        */
        void setupGetThread(MCount cnt);

        /**
        @version 0.9.1
        */
        Thread * getPostThread() const;

        /**
        @version 0.9.1
        */
        Thread * getGetThread() const;

        /**
        @version 0.9.1
        */
        void response(ServerConnect * connect, char * data, int size);


    protected:
        FileOpManager();
        FileOpManager(const FileOpManager &);
        ~FileOpManager();

        /// @copydetails TimerListener::onTimer
        void onTimer(TimeDurMS tick);
        
        int createEntry(const String & url, size_t filesize, const void * content);

        FileEntry * getEntry(const String & url, bool create);

        void destroyEntry(const String & url);

        void recoverEntry();
    protected:
        typedef std::map<String, FileEntry *> EntryMap;
    private:
        String mServerName;
        String mFilePath;
        Mui32 mPerDirCount;
        Mui64 mFileCount;
        ACE_Thread_Mutex mFileCountMute;
        ACE_Thread_Mutex mFileMute;
        EntryMap mFileEntryList;
        Thread * mPostThread;
        Thread * mGetThread;
        ACE_Reactor * mReactor;
        ACE_Thread_Mutex mResponseMutex;
        list<Response *> mResponseList;
    };
    M_SingletonDef(FileOpManager);
}
#endif