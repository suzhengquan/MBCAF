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

#ifndef WIN32
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

#include "MdfFileOp.h"
#if (__linux__)
    #include <sys/sendfile.h>
#elif (__FREEBSD__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/uio.h>
#elif(WIN32)
    #include <io.h>
#endif

namespace Mdf
{
    //-----------------------------------------------------------------------
    FileOp::FileOp(const String & path)
    {
        mFilePath = path;
        mFileHandle = ACE_INVALID_HANDLE;
        mSize = -1;
        mOpen = false;
    }
    //-----------------------------------------------------------------------
    FileOp::~FileOp()
    {
        if(mOpen)
        {
            close();
        }
    }
    //-----------------------------------------------------------------------
    const String & FileOp::getPath() const
    {
        return mFilePath;
    }
    //-----------------------------------------------------------------------
    bool FileOp::isExist() const
    {
        return ACE_OS::access(mFilePath, 0) == 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::remove() 
    {
        if(-1 == ACE_OS::unlink(mFilePath))
            return errno;
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::create(bool directIo)
    {
        assert(!mOpen);
        int flags = O_RDWR | O_CREAT | O_EXCL;
    #if defined(WIN32)
        mFileHandle = ACE_OS::open(mFilePath, flags);
    #else
        mFileHandle = ACE_OS::open(mFilePath, flags, 00640);
    #endif    
        if (ACE_INVALID_HANDLE == mFileHandle)
            return errno;
    #ifdef __LINUX__ || __FREEBSD__
        if (directIo)
        {
            if (-1 == fcntl(mFileHandle, F_SETFL, O_DIRECT))
                return errno;
        }
    #endif    
        mOpen = true;
        mSize = 0;
        mDirect = directIo;
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::open(bool directIo)
    {
        assert(!mOpen);
        int flags = O_RDWR;
        mFileHandle = ACE_OS::open(mFilePath, flags);
        if (ACE_INVALID_HANDLE == mFileHandle)
        {
            return errno;
        }
    #ifdef __LINUX__ || __FREEBSD__
        if (directIo)
        {
            if (-1 == fcntl(mFileHandle, F_SETFL, O_DIRECT))
                return errno;
        }
    #endif
        ACE_File_Lock lock(mFileHandle, false);
        if (lock.acquire(SEEK_SET, 0, 0) < 0)
        {
            ACE_OS::close(mFileHandle);
            return errno;
        }

        mOpen = true;
        Mui64 size = getSize();
        if(size == -1)
        {
            close();
            return -1;
        }
        mSize = size;
        mDirect = directIo;
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::close() 
    {
        if(!mOpen)
            return 0;

        if (ACE_INVALID_HANDLE == ACE_OS::fsync(mFileHandle))
        {
            return errno;
        }

        mOpen = false;
        if(0 != ACE_OS::close(mFileHandle))
            return errno;
        return 0;        
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::getSize() const
    {
        if (mSize > 0) 
        {
            return mSize;
        }

        Mi64 sksize = ACE_OS::lseek(mFileHandle, 0, SEEK_END);
        if (-1 == sksize)
            return -1;
        mSize = *sksize;
        return mSize;
    }
    //-----------------------------------------------------------------------
    bool FileOp::setSize(Mui64 size) 
    {
        assert(mOpen);
        if (-1 == ACE_OS::ftruncate(mFileHandle, size))
            return false;
        mSize = size;
        return true;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::read(Mui32 size, void * buffer) 
    {
        assert(mOpen);
        if (size > (Mui64)mSize)
            return 0;
        if (size != ACE_OS::read(mFileHandle, buffer, size))
            return errno;
        return 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::write(Mui32 size, const void * buffer) 
    {
        assert(mOpen);
        setSize((Mui64)size);
        if (size > (Mui64)mSize)
            return 0;

        if(size != ACE_OS::write(mFileHandle, buffer, size))
            return errno;
        return 0;
    }
    //-----------------------------------------------------------------------
    bool FileOp::isDirectory() const
    {
        ACE_stat fileStat;
        if(ACE_OS::stat(mFilePath, &fileStat) != 0)
            return false;

        return S_ISDIR(fileStat.st_mode) != 0;
    }
    //-----------------------------------------------------------------------
    Mui64 FileOp::getFileCount() const
    {
        ACE_Dirent dp;
        ACE_DIRENT * ep = NULL;
        String dir = mFilePath;
        if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
            dir += "/";

        int re = dp.open(mFilePath);
        if (re != 0)
            return 0;
        int files = 0;
        ep = dp.read();
        while(ep) 
        {
            String subPath = dir + ep->d_name;
            FileOp subFile(subPath.c_str());
            bool isDir = false;
            if(subFile.isDirectory(&isDir) != 0 || isDir) 
            {
                dp.close();
                return 0;
            } 
            else 
            {
                if (strncmp(ep->d_name, ".", 1) != 0)
                {
                    files++;
                }
            }
            ep = dp.read();
        }
        dp.close();

        return files;
    }
    //-----------------------------------------------------------------------
}
#endif
