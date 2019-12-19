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
#include "MdfThreadManager.h"
#include "MdfFileOp.h"

#define MAX_FILE_SIZE_PER_FILE (5 * 1024 * 1024)
#define FIRST_DIR_MAX 255
#define SECOND_DIR_MAX 255
#define MAX_FILE_IN_MAP 10000

namespace Mdf
{
    //-----------------------------------------------------------------------
    typedef struct Response_t
    {
        ServerConnect * mConnect;
        char * mData;
        uint32_t mSize;
    } Response;
    //-----------------------------------------------------------------------
    M_SingletonImpl(FileOpManager);
    //-----------------------------------------------------------------------
    class FileEntry
    {
    public:
        FileEntry()
        {
            mAcess = 0;
            mSize = 0;
            mData = NULL;
        }
        ~FileEntry()
        {
            if(mData)
            {
                free(mData);
                mData = NULL;
            }
        }
    public:
        time_t mAcess;
        size_t mSize;
        Mui8 * mData;
    };
    //-----------------------------------------------------------------------
    FileOpManager::FileOpManager():
        mPostThread(0),
        mGetThread(0),
        mReactor(0)
    {
        setEnable(true);
    }
    //-----------------------------------------------------------------------
    FileOpManager::~FileOpManager()
    {
        EntryMap::iterator i, iend = mFileEntryList.end();
        for(i = mFileEntryList.begin(); i != iend; ++i)
        {
            delete it->second;
        }
        mFileEntryList.clear();
        setEnable(false);
    }
    //-----------------------------------------------------------------------
    Mi64 FileOpManager::getFileSize(const String & path)
    {
        Mi64 filesize = -1;
        ACE_stat statbuff;
        if (ACE_OS::stat(path, &statbuff) < 0)
        {
            return filesize;
        }
        else
        {
            filesize = statbuff.st_size;
        }
        return filesize;
    }
    //-----------------------------------------------------------------------
    bool FileOpManager::isExist(const String & path)
    {
        return FileOp(path).isExist();
    }
    //-----------------------------------------------------------------------
    bool FileOpManager::setInfo(const String & host, const String & disk, int total, int perDirCnt)
    {
        mServerName = host;
        mFilePath = disk;
        mFileCount = total;
        mPerDirCount = perDirCnt;

        bool isExist = isExist(mFilePath);
        if (!isExist)
        {
            if (ACE_OS::mkdir(mFilePath, 0777) != 0)
            {
                Mlog("The dir[%s] set error for code[%d], its parent dir may no exists", 
                    mFilePath, errno);
                return -1;
            }
        }

        char first[10] = { 0 };
        char second[10] = { 0 };
        for (int i = 0; i <= FIRST_DIR_MAX; ++i)
        {
            snprintf(first, 5, "%03d", i);
            String tmp = mFilePath + "/" + String(first);
            if ((ACE_OS::mkdir(tmp, 0777) != 0) && (errno != EEXIST))
            {
                Mlog("Create dir[%s] error[%d]", tmp.c_str(), errno);
                return -1;
            }
            for (int j = 0; j <= SECOND_DIR_MAX; ++j)
            {
                snprintf(second, 5, "%03d", j);
                String tmp2 = tmp + "/" + String(second);
                if ((ACE_OS::mkdir(tmp2, 0777) != 0) && (errno != EEXIST))
                {
                    Mlog("Create dir[%s] error[%d]", tmp2.c_str(), errno);
                    return -1;
                }
                memset(second, 0x0, 10);
            }
            memset(first, 0x0, 10);
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOpManager::getFileCount()
    { 
        return mFileCount; 
    }
    //-----------------------------------------------------------------------
    String FileOpManager::getRelPath() 
    {
        char first[10] = {0};
        char second[10] = {0};
        mFileMute.acquire();
        snprintf(first, 5, "%03d", (int)(mFileCount / (mPerDirCount)) / (FIRST_DIR_MAX));
        snprintf(second, 5, "%03d", (int)((mFileCount % (mPerDirCount * FIRST_DIR_MAX)) / mPerDirCount));
        mFileMute.release();

        struct timeval tv;
        gettimeofday(&tv,NULL);
        Mui64 usec = tv.tv_sec * 1000000 + tv.tv_usec;
        Mui64 tid = (Mui64)ACE_OS::thr_self();
        char unique[40];
        snprintf(unique, 30, "%llu_%llu", usec, tid);
        String path = "/" + String(first) + "/" + String(second) + "/" + String(unique);
        return String(path);
    }
    //-----------------------------------------------------------------------
    int FileOpManager::uploadFile(String & url, const char * type, const void * content,
        Mui32 size, char * ext) 
    {
        if (size > MAX_FILE_SIZE_PER_FILE) 
        {
            Mlog("FileOp size[%d] should less than [%d]", size, MAX_FILE_SIZE_PER_FILE);
            return -1;
        }

        String path = getRelPath();
        if (ext)
            path += "_" + String(ext);
        else
            path += "." + String(type);
    
        String groups("g0");
        url = groups + path;

        mFileMute.acquire();
        createEntry(url, (Mui64)size, content);
        recoverEntry();
        mFileMute.release();

        String absPath = mFilePath + path;
        FileOp * tmpFile = new FileOp(absPath.c_str());
        tmpFile->create();
        tmpFile->write(size, content);
        delete tmpFile;

        mFileCountMute.acquire();
        mFileCount++;
        mFileCountMute.release();
    
        return 0;
    }
    //-----------------------------------------------------------------------
    int FileOpManager::getURLRelPath(const String & url, String & path) 
    {
        String::size_type pos = url.find("/");
        if(String::npos == pos) 
        {
            Mlog("Url [%s] format illegal.", url.c_str());
            return -1;
        }
        path = url.substr(pos);
        return 0;
    }
    //-----------------------------------------------------------------------
    int FileOpManager::getURLAbsPath(const String & url, String & path) 
    {
        String relate;
        if(getURLRelPath(url, relate)) 
        {
            Mlog("Get path from url[%s] error", url.c_str());
            return -1;
        }
        path = mFilePath + relate;
        return 0;
    }
    //-----------------------------------------------------------------------
    void FileOpManager::response(ServerConnect * connect, char * data, int size)
    {
        Response * a = new Response;
        a->mConnect = connect;
        a->mData = data;
        a->mSize = size;

        mResponseMutex.acquire();
        mResponseList.push_back(a);
        mResponseMutex.release();
    }
    //-----------------------------------------------------------------------
    void FileOpManager::setupPostThread(MCount cnt)
    {
        mPostThread = M_Only(ThreadManager)->create(cnt);
    }
    //-----------------------------------------------------------------------
    void FileOpManager::setupGetThread(MCount cnt)
    {
        mGetThread = M_Only(ThreadManager)->create(cnt);
    }
    //-----------------------------------------------------------------------
    ThreadPool * FileOpManager::getPostThread() const
    {
        return mPostThread;
    }
    //-----------------------------------------------------------------------
    ThreadPool * FileOpManager::getGetThread() const
    {
        return mGetThread;
    }
    //-----------------------------------------------------------------------
    int FileOpManager::downloadFile(const String & url, void * buf, Mui32 * size)
    {
        FileEntry * en = getEntry(url, true);
        if(!en) 
        {
            Mlog("download file error, while url:%s", url);
            return -1;
        }
        memcpy(buf, en->mData, en->mSize);
        *size = (Mui32)en->mSize;
        en->mAcess = time(NULL);
        return 0;
    }
    //-----------------------------------------------------------------------
    void FileOpManager::recoverEntry() 
    {
        size_t currSize = mFileEntryList.size();
        if (currSize > MAX_FILE_IN_MAP) 
        {
            EntryMap::iterator it = mFileEntryList.begin();
            int times = abs(MAX_FILE_IN_MAP - currSize);
            while (it != mFileEntryList.end() && times) 
            {
                delete it->second;
                mFileEntryList.erase(it++);
                times--;
            }

            it = mFileEntryList.begin();
            while (it != mFileEntryList.end() && times) 
            {
                time_t currTime = time(NULL);
                if(currTime - it->second->mAcess > 2*60*60) 
                {
                    delete it->second;
                    mFileEntryList.erase(it++);
                }
            }
        }
    }
    //-----------------------------------------------------------------------
    int FileOpManager::createEntry(const String & url, size_t size, const void * data)
    {
        if(mFileEntryList.size()) 
        {
            EntryMap::iterator it = mFileEntryList.find(url);
            if (it != mFileEntryList.end()) 
                return -1;
        }
        FileEntry * e = new FileEntry();
        e->mSize = size;
        e->mData = (Mui8 *)malloc(size);
        e->mAcess = time(NULL);
        memcpy(e->mData, data, size);
    
        mFileMute.acquire();
        pair<map<std::String, FileEntry *>::iterator, bool> ret;
        ret = mFileEntryList.insert(EntryMap::value_type(url, e));
        if (ret.second == false) 
        {
            delete e;
            e = NULL;
        }
        recoverEntry();
        mFileMute.release();
        return 0;
    }
    //-----------------------------------------------------------------------
    FileOpManager::FileEntry * FileOpManager::getEntry(const String & url, bool create)
    {
        mFileMute.acquire();
        EntryMap::iterator it = mFileEntryList.find(url);
        if (it != mFileEntryList.end())
        {
            Mlog("the map has the file while url:%s", url.c_str());
            mFileMute.release();
            return it->second;
        }
        if (!create)
        {
            mFileMute.release();
            return NULL;
        }

        String path;
        if (getURLAbsPath(url, path))
        {
            Mlog("Get abs path from url[%s] error", url.c_str());
            mFileMute.release();
            return NULL;
        }

        ACE_stat buf;
        if (ACE_OS::stat(path.c_str(), &buf) == -1)
        {
            mFileMute.release();
            return NULL;
        }

        if (!S_ISREG(buf.st_mode))
        {
            mFileMute.release();
            return NULL;
        }

        FileEntry * e = new FileEntry();
        Mui64 fileSize = getFileSize(path);
        e->mSize = fileSize;
        e->mData = (Mui8 *)malloc(fileSize);
        memset(e->mData, 0x0, fileSize);
        e->mAcess = time(NULL);
        FileOp * tmpFile = new FileOp(path.c_str());
        tmpFile->open();
        int ret = tmpFile->read(fileSize, e->mData);
        if (ret)
        {
            Mlog("read file error while url:%s", url.c_str());
            delete e;
            delete tmpFile;
            mFileMute.release();
            return NULL;
        }
        delete tmpFile;

        std::pair < map <std::String, FileEntry *>::iterator, bool> result;
        result = mFileEntryList.insert(EntryMap::value_type(url, e));
        if (result.second == false)
        {
            Mlog("Insert url[%s] to file map error", url.c_str());
            delete e;
            e = NULL;
        }
        recoverEntry();
        mFileMute.release();
        return e;
    }
    //-----------------------------------------------------------------------
    void FileOpManager::destroyEntry(const String & url) 
    {
        mFileMute.acquire();
        const FileEntry * entry = const_cast<FileOpManager *>(this)->getEntry(url, false);
        if(!entry) 
        {
            Mlog("map has not the url::%s", url.c_str());
            mFileMute.release();
            return;
        }
        free(entry->mData);
        mFileEntryList.erase(url);
        mFileMute.release();
        return;
    }
    //-----------------------------------------------------------------------
    void FileOpManager::onTimer(TimeDurMS tick)
    {
        mResponseMutex.acquire();
        while (!mResponseList.empty())
        {
            Response * a = mResponseList.front();
            mResponseList.pop_front();
            mResponseMutex.release();

            if (a != NULL)
            {
                ServerConnect * pConn = a->mConnect;
                if (pConn)
                {
                    pConn->send(a->mData, a->mSize);
                    pConn->stop();
                }
                if (a->mData != NULL)
                {
                    free(a->mData);
                    a->mData = NULL;
                }
                delete a;
            }
            mResponseMutex.acquire();
        }
        mResponseMutex.release();
    }
    //-----------------------------------------------------------------------
}

