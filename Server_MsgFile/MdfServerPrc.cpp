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

#include "MdfServerPrc.h"
#include "MdfMessage.h"
#include "MdfThreadManager.h"
#include "MdfFileManager.h"
#include "MdfServerConnect.h"

/**
 * This macro is used to increase the invocation counter by one when entering
 * handle_input(). It also checks wether the counter is greater than zero
 * indicating, that handle_input() has been called before.
 */
#define INVOCATION_ENTER()            \
{                                    \
    if (mDebugMark > 0)                \
        logError(ACE_TEXT("Multiple invocations detected.\n")); \
    mDebugMark++;                    \
}

/**
 * THis macro is the counter part to INVOCATION_ENTER(). It decreases the
 * invocation counter and then returns the given value. This macro is
 * here for convenience to decrease the invocation counter also when returning
 * due to errors.
 */
#define INVOCATION_RETURN(retval) { mDebugMark--; return retval; }

#define HTTP_UPLOAD_MAX        0xA00000
#define BOUNDARY_MARK        "boundary="
#define HTTP_END_MARK        "\r\n\r\n"
#define CONTENT_TYPE        "Content-Type:"
#define CONTENT_DISPOSITION    "Content-Disposition:"
#define M_SocketBlockSize    0x100000
#define HTTP_A_Header        "HTTP/1.1 200 OK\r\nConnection:close\r\n"            \
                            "Content-Length:%d\r\nContent-Type:multipart/form-data\r\n\r\n"
#define HTTP_A_Extend        "HTTP/1.1 200 OK\r\nConnection:close\r\n"            \
                            "Content-Length:%d\r\nContent-Type:multipart/form-data\r\n\r\n"
#define HTTP_A_Image        "HTTP/1.1 200 OK\r\nConnection:close\r\n"            \
                            "Content-Length:%d\r\nContent-Type:image/%s\r\n\r\n"
#define HTTP_A_403            "HTTP/1.1 403 Access Forbidden\r\nContent-Length: 0\r\n"            \
                            "Connection: close\r\nContent-Type: text/html;charset=utf-8\r\n\r\n"
#define HTTP_A_403_Size        strlen(HTTP_A_403)
#define HTTP_A_404            "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n"    \
                            "Connection: close\r\nContent-Type: text/html;charset=utf-8\r\n\r\n"
#define HTTP_A_404_Size        strlen(HTTP_A_404)
#define HTTP_A_500            "HTTP/1.1 500 Internal Server Error\r\nConnection:close\r\n"            \
                            "Content-Length:0\r\nContent-Type:text/html;charset=utf-8\r\n\r\n"
#define HTTP_A_500_Size        strlen(HTTP_A_500)
#define HTTP_A_Text            "HTTP/1.1 200 OK\r\nConnection:close\r\nContent-Length:%d\r\n"\
                            "Content-Type:text/html;charset=utf-8\r\n\r\n%s"
#define HTTP_A_MaxSize        1024

namespace Mdf
{
    //-----------------------------------------------------------------------
    class Request
    {
    public:
        String mURL;
        String mHost;
        String mContentType;
        ServerConnect * mConnnect;
        char * mContent;
        int mMethod;
        int mLength;
    };
    //-----------------------------------------------------------------------
    class FileHttpMain : public ThreadMain, public Request
    {
    public:
        FileHttpMain(Request q);
        virtual ~FileHttpMain();
        void run();
        void onUpload();
        void onDownload();
    };
    //-----------------------------------------------------------------------
    const char * memfind(const char * src, size_t srcsize,
        const char * sub, size_t subsize, bool flag)
    {
        if (NULL == src || NULL == sub || srcsize <= 0)
        {
            return NULL;
        }
        if (srcsize < subsize)
        {
            return NULL;
        }
        const char * p;
        if (subsize == 0)
        {
            subsize = strlen(sub);
        }
        if (srcsize == subsize)
        {
            if (0 == (memcmp(src, sub, srcsize)))
            {
                return src;
            }
            else
            {
                return NULL;
            }
        }
        if (flag)
        {
            for (int i = 0; i < srcsize - subsize; i++)
            {
                p = src + i;
                if (0 == memcmp(p, sub, subsize))
                    return p;
            }
        }
        else
        {
            for (int i = (srcsize - subsize); i >= 0; i--)
            {
                p = src + i;
                if (0 == memcmp(p, sub, subsize))
                    return p;

            }
        }
        return NULL;
    }
    //-----------------------------------------------------------------------
    FileHttpMain::FileHttpMain(Request q)
    {
        mURL = q.mURL;
        mHost = q.mHost;
        mMethod = q.mMethod;
        mLength = q.mLength;
        mContent = q.mContent;
        mConnnect = q.mConnnect;
        mContentType = q.mContentType;
    }
    //-----------------------------------------------------------------------
    FileHttpMain::~FileHttpMain()
    {
    }
    //-----------------------------------------------------------------------
    void FileHttpMain::run()
    {
        if (HTTP_GET == mMethod)
        {
            onDownload();
        }
        else if (HTTP_POST == mMethod)
        {
            onUpload();
        }
        else
        {
            char * content = (char *)malloc(strlen(HTTP_A_403));
            snprintf(content, strlen(HTTP_A_403), HTTP_A_403);
            M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
        }
        if (mContent != NULL)
        {
            free(mContent);
            mContent = NULL;
        }
    }
    //-----------------------------------------------------------------------
    void FileHttpMain::onUpload()
    {
        int nTmpLen = 0;
        char * content = NULL;
        const char * pPos = memfind(mContent, mLength, CONTENT_DISPOSITION, strlen(CONTENT_DISPOSITION));
        if (pPos != NULL)
        {
            char url[128];
            nTmpLen = pPos - mContent;
            const char * pPos2 = memfind(pPos, mLength - nTmpLen, "filename=", strlen("filename="));
            if (pPos2 != NULL)
            {
                pPos = pPos2 + strlen("filename=") + 1;
                const char * pPosQuotes = memfind(pPos, mLength - nTmpLen, "\"", strlen("\""));
                int nFileNameLen = pPosQuotes - pPos;

                char szFileName[256];
                if (nFileNameLen <= 255)
                {
                    memcpy(szFileName, pPos, nFileNameLen);
                    szFileName[nFileNameLen] = 0;

                    const char * pPosType = memfind(szFileName, nFileNameLen, ".", 1, false);
                    if (pPosType != NULL)
                    {
                        char szType[16];
                        int nTypeLen = nFileNameLen - (pPosType + 1 - szFileName);
                        if (nTypeLen <= 15)
                        {
                            memcpy(szType, pPosType + 1, nTypeLen);
                            szType[nTypeLen] = 0;
                            Mlog("upload file, file name:%s", szFileName);
                            char szExtend[16];
                            const char * pPosExtend = memfind(szFileName, nFileNameLen, "_", 1, false);
                            if (pPosExtend != NULL)
                            {
                                const char * pPosTmp = memfind(pPosExtend, nFileNameLen - (pPosExtend + 1 - szFileName), "x", 1);
                                if (pPosTmp != NULL)
                                {
                                    int nWidthLen = pPosTmp - pPosExtend - 1;
                                    int nHeightLen = pPosType - pPosTmp - 1;
                                    if (nWidthLen >= 0 && nHeightLen >= 0)
                                    {
                                        int nWidth = 0;
                                        int nHeight = 0;
                                        char szWidth[5], szHeight[5];
                                        if (nWidthLen <= 4 && nHeightLen <= 4)
                                        {
                                            memcpy(szWidth, pPosExtend + 1, nWidthLen);
                                            szWidth[nWidthLen] = 0;
                                            memcpy(szHeight, pPosTmp + 1, nHeightLen);
                                            szHeight[nHeightLen] = 0;
                                            nWidth = atoi(szWidth);
                                            nHeight = atoi(szHeight);
                                            snprintf(szExtend, sizeof(szExtend), "%dx%d.%s", nWidth, nHeight, szType);
                                        }
                                        else
                                        {
                                            szExtend[0] = 0;
                                        }
                                    }
                                    else
                                    {
                                        szExtend[0] = 0;
                                    }
                                }
                                else
                                {
                                    szExtend[0] = 0;
                                }
                            }
                            else
                            {
                                szExtend[0] = 0;
                            }

                            size_t nPos = mContentType.find(BOUNDARY_MARK);
                            if (nPos != mContentType.npos)
                            {
                                const char * pBoundary = mContentType.c_str() + nPos + strlen(BOUNDARY_MARK);
                                int nBoundaryLen = mContentType.length() - nPos - strlen(BOUNDARY_MARK);

                                pPos = memfind(mContent, mLength, pBoundary, nBoundaryLen);
                                if (NULL != pPos)
                                {
                                    nTmpLen = pPos - mContent;
                                    pPos = memfind(mContent + nTmpLen, mLength - nTmpLen, CONTENT_TYPE, strlen(CONTENT_TYPE));
                                    if (NULL != pPos)
                                    {
                                        nTmpLen = pPos - mContent;
                                        pPos = memfind(mContent + nTmpLen, mLength - nTmpLen, HTTP_END_MARK, strlen(HTTP_END_MARK));
                                        if (NULL != pPos)
                                        {
                                            nTmpLen = pPos - mContent;
                                            const char * pFileStart = pPos + strlen(HTTP_END_MARK);
                                            pPos2 = memfind(mContent + nTmpLen, mLength - nTmpLen, pBoundary, nBoundaryLen);
                                            if (NULL != pPos2)
                                            {
                                                int64_t nFileSize = pPos2 - strlen(HTTP_END_MARK) - pFileStart;
                                                if (nFileSize <= HTTP_UPLOAD_MAX)
                                                {
                                                    String filePath;
                                                    if (strlen(szExtend) != 0)
                                                    {
                                                        M_Only(FileOpManager)->uploadFile(filePath, szType, pFileStart, nFileSize, szExtend);
                                                    }
                                                    else
                                                    {
                                                        M_Only(FileOpManager)->uploadFile(filePath, szType, pFileStart, nFileSize);
                                                    }
                                                    char url[1024];
                                                    snprintf(url, sizeof(url), "{\"error_code\":0,\"error_msg\": \"success\",\"path\":\"%s\",\"url\":\"http://%s/%s\"}", 
                                                        filePath.c_str(), mHost.c_str(), filePath.c_str());
                                                    uint32_t dlen = strlen(url);
                                                    content = (char *)malloc(HTTP_A_MaxSize);
                                                    snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                                    M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                                                }
                                            }
                                            else
                                            {
                                                snprintf(url, sizeof(url), "{\"error_code\":8,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                                                Mlog("%s", url);
                                                uint32_t dlen = strlen(url);
                                                content = (char *)malloc(HTTP_A_MaxSize);
                                                snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                                M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                                            }
                                        }
                                        else
                                        {
                                            snprintf(url, sizeof(url), "{\"error_code\":7,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                                            Mlog("%s", url);
                                            uint32_t dlen = strlen(url);
                                            content = (char *)malloc(HTTP_A_MaxSize);
                                            snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                            M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                                        }

                                    }
                                    else
                                    {
                                        snprintf(url, sizeof(url), "{\"error_code\":6,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                                        Mlog("%s", url);
                                        uint32_t dlen = strlen(url);
                                        content = (char *)malloc(HTTP_A_MaxSize);
                                        snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                        M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                                    }
                                }
                                else
                                {
                                    snprintf(url, sizeof(url), "{\"error_code\":5,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                                    Mlog("%s", url);
                                    uint32_t dlen = strlen(url);
                                    content = (char *)malloc(HTTP_A_MaxSize);
                                    snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                    M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                                }
                            }
                            else
                            {
                                snprintf(url, sizeof(url), "{\"error_code\":4,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                                Mlog("%s", url);
                                uint32_t dlen = strlen(url);
                                content = (char *)malloc(HTTP_A_MaxSize);
                                snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                                M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                            }
                        }
                        else
                        {
                            snprintf(url, sizeof(url), "{\"error_code\":9,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                            Mlog("%s", url);
                            uint32_t dlen = strlen(url);
                            content = (char *)malloc(HTTP_A_MaxSize);
                            snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                            M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                        }
                    }
                    else
                    {
                        snprintf(url, sizeof(url), "{\"error_code\":10,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                        Mlog("%s", url);
                        uint32_t dlen = strlen(url);
                        content = (char *)malloc(HTTP_A_MaxSize);
                        snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                        M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                    }
                }
                else
                {
                    snprintf(url, sizeof(url), "{\"error_code\":11,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                    Mlog("%s", url);
                    uint32_t dlen = strlen(url);
                    content = (char *)malloc(HTTP_A_MaxSize);
                    snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                    M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
                }
            }
            else
            {
                snprintf(url, sizeof(url), "{\"error_code\":3,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
                Mlog("%s", url);
                uint32_t dlen = strlen(url);
                content = (char *)malloc(HTTP_A_MaxSize);
                snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
                M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
            }
        }
        else
        {
            snprintf(url, sizeof(url), "{\"error_code\":2,\"error_msg\": \"error format\",\"path\":\"\",\"url\":\"\"}");
            Mlog("%s", url);
            uint32_t dlen = strlen(url);
            content = (char *)malloc(HTTP_A_MaxSize);
            snprintf(content, HTTP_A_MaxSize, HTTP_A_Text, dlen, url);
            M_Only(FileOpManager)->response(mConnnect, content, strlen(content));
        }
    }
    //-----------------------------------------------------------------------
    void FileHttpMain::onDownload()
    {
        uint32_t nFileSize = 0;
        int32_t nTmpSize = 0;
        String strPath;
        if (M_Only(FileOpManager)->getURLAbsPath(mURL, strPath) == 0)
        {
            nTmpSize = getFileSize(strPath);
            if (nTmpSize != -1)
            {
                char header[1024];
                size_t nPos = strPath.find_last_of(".");
                String strType = strPath.substr(nPos + 1, strPath.length() - nPos);
                if (strType == "jpg" || strType == "JPG" || strType == "jpeg" || strType == "JPEG" ||
                    strType == "png" || strType == "PNG" || strType == "gif" || strType == "GIF")
                {
                    snprintf(header, sizeof(header), HTTP_A_Image, nTmpSize, strType.c_str());
                }
                else
                {
                    snprintf(header, sizeof(header), HTTP_A_Extend, nTmpSize);
                }
                int hsize = strlen(header);
                char * content = (char *)malloc(hsize + nTmpSize);
                memcpy(content, header, hsize);
                M_Only(FileOpManager)->downloadFile(mURL, content + hsize, &nFileSize);
                int total = hsize + nFileSize;
                M_Only(FileOpManager)->response(mConnnect, content, total);
            }
            else
            {
                int total = strlen(HTTP_A_404);
                char * content = (char *)malloc(total);
                snprintf(content, total, HTTP_A_404);
                M_Only(FileOpManager)->response(mConnnect, content, total);
            }
        }
        else
        {
            int total = strlen(HTTP_A_500);
            char * content = (char *)malloc(total);
            snprintf(content, total, HTTP_A_500);
            M_Only(FileOpManager)->response(mConnnect, content, total);
        }
    }
    //-----------------------------------------------------------------------
    ServerPrc::ServerPrc():
        SocketServerPrc()
    {
        M_Trace("ServerPrc::ServerPrc()");
    }
    //-----------------------------------------------------------------------
    ServerPrc::ServerPrc(ServerIO * prc, ACE_Reactor * reactor) :
        SocketServerPrc(prc, reactor)
    {
        M_Trace("ServerPrc::ServerPrc()");
    }
    //-----------------------------------------------------------------------
    ServerPrc::~ServerPrc()
    {
        M_Trace("ServerPrc::~ServerPrc()");
    }
    //-----------------------------------------------------------------------
    SocketServerPrc * ServerPrc::createInstance(ACE_Reactor * tor) const
    {
        ServerIO * base = mBase->createInstance();
        SocketServerPrc * re = new ServerPrc(base, tor);
        base->setup(re);

        return re;
    }
    //-----------------------------------------------------------------------
    int ServerPrc::handle_input(ACE_HANDLE)
    {
        M_Trace("ServerPrc::handle_input(ACE_HANDLE)");

        INVOCATION_ENTER();

        while(1)
        {
            Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
            if(bufresever < M_SocketBlockSize + 1)
                mReceiveBuffer.enlarge(M_SocketBlockSize + 1);

            int recsize = mBase->getStream()->recv(mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);
            if (recsize <= 0)
            {
                if(ACE_OS::last_error() != EWOULDBLOCK)
                    INVOCATION_RETURN(-1);
                else
                    break;
            }
            mReceiveBuffer.writeSkip(recsize);
            mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        }

        char * in_buf = (char *)mReceiveBuffer.getBuffer();
        uint32_t buf_len = mReceiveBuffer.getWriteSize();
        in_buf[buf_len] = '\0';

        mHttpParser.parse(in_buf, buf_len);

        if (mHttpParser.isComplete())
        {
            String mURL = mHttpParser.getUrl();
            Mlog("IP:%s access:%s", mIP.c_str(), mURL.c_str());
            if (mURL.find("..") != mURL.npos)
            {
                stop();
                INVOCATION_RETURN(-1);
            }

            if (mHttpParser.getContentLength() > HTTP_UPLOAD_MAX)
            {
                char url[128];
                snprintf(url, sizeof(url), "{\"error_code\":1,\"error_msg\": \"file too big\",\"url\":\"\"}");
                Mlog("%s", url);
                uint32_t dlen = strlen(url);
                char content[1024];
                snprintf(content, sizeof(content), HTTP_A_Text, dlen, url);
                send(content, strlen(content));
                stop();
                INVOCATION_RETURN(-1);
            }

            int clen = mHttpParser.getContentLength();
            char * content = NULL;
            if (clen != 0)
            {
                try
                {
                    content = (char *)malloc(clen);
                    memcpy(content, mHttpParser.getBodyContent(), clen);
                }
                catch (...)
                {
                    char szResponse[HTTP_A_500_Size + 1];
                    snprintf(szResponse, HTTP_A_500_Size, "%s", HTTP_A_500);
                    send(szResponse, HTTP_A_500_Size);
                    stop();
                    INVOCATION_RETURN(-1);
                }
            }
            Request q;
            q.mConnnect = static_cast<ServerConnect *>(getBase());
            q.mLength = clen;
            q.mContent = mContent;
            q.mHost = mHttpParser.getHost();
            q.mMethod = mHttpParser.getMethod();
            q.mContentType = mHttpParser.getContentType();
            q.mURL = mHttpParser.getUrl();
            FileHttpMain * pTask = new FileHttpMain(q);
            if (HTTP_GET == mHttpParser.getMethod())
            {
                M_Only(FileOpManager)->getGetThread()->add(pTask);
            }
            else
            {
                M_Only(FileOpManager)->getPostThread()->add(pTask);
            }
        }
        INVOCATION_RETURN(-1);
    }
    //-----------------------------------------------------------------------
    int ServerPrc::handle_output(ACE_HANDLE fd)
    {
        M_Trace("ServerPrc::handle_output(ACE_HANDLE)");
        INVOCATION_ENTER();

    ContinueTransmission:
        ACE_Message_Block * mb = 0;
        ACE_Time_Value nowait(ACE_OS::gettimeofday());
        while (0 <= mOutQueue.dequeue_head(mb, &nowait))
        {
            int dsedsize = mBase->getStream()->send(mb->rd_ptr(), mb->length());
            if (dsedsize <= 0)
            {
                if (ACE_OS::last_error() != EWOULDBLOCK)
                {
                    INVOCATION_RETURN(-1);
                }
                else
                {
                    mOutQueue.enqueue_head(mb);
                    break;
                }
            }
            else
            {
                mb->rd_ptr((size_t)dsedsize);
            }
            if (mb->length() > 0)
            {
                mOutQueue.enqueue_head(mb);
                break;
            }
            mb->release();
            mSendMark = M_Only(ConnectManager)->getTimeTick();
        }
        if (mBase->isStop())
        {
            if (!mOutQueue.is_empty())
            {
                ACE_OS::sleep(1);
                goto ContinueTransmission;
            }
            INVOCATION_RETURN(-1);
        }
        {
            ScopeLock tlock(mOutMute);
            if (mOutQueue.is_empty())
                mReactor->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
            else
                mReactor->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
        }
        INVOCATION_RETURN(0);
    }
    //-----------------------------------------------------------------------
}