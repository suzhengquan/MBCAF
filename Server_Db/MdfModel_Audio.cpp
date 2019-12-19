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

#include "MdfDatabaseManager.h"
#include "MdfModel_Audio.h"
#include "json/json.h"

namespace Mdf
{
    //----------------------------------------------------------------
    size_t write_data_string(void * ptr, size_t size, size_t cnt, void * out)
    {
        size_t len = size * cnt;
        String * re = (String *)out;
        re->append((char *)ptr, len);
        return len;
    }
    //----------------------------------------------------------------
    size_t write_data_binary(void * ptr, size_t size, size_t cnt, AudioMsgInfo * out)
    {
        size_t len = size * cnt;
        if (out->data_len + len <= out->fileSize + 4)
        {
            memcpy(out->data + out->data_len, ptr, len);
            out->data_len += len;
        }
        return len;
    }
    //----------------------------------------------------------------
    static size_t OnWriteData(void * buffer, size_t size, size_t cnt, void * out)
    {
        String * str = dynamic_cast<String *>((String *)out);
        if (NULL == str || NULL == buffer)
        {
            return -1;
        }
        size_t len = size * cnt;
        char * data = (char *)buffer;
        str->append(data, len);
        return len;
    }
    //-----------------------------------------------------------------------
    M_SingletonImpl(Model_Audio);
    //-----------------------------------------------------------------------
    Model_Audio::Model_Audio()
    {
    }
    //-----------------------------------------------------------------------
    Model_Audio::~Model_Audio()
    {
    }
    //-----------------------------------------------------------------------
    void Model_Audio::setUrl(String & url)
    {
        mFileURL = url;
        if (mFileURL[mFileURL.length()] != '/')
        {
            mFileURL += "/";
        }
    }
    //-----------------------------------------------------------------------
    bool Model_Audio::readAudios(list<MBCAF::Proto::MsgInfo> & infolist)
    {
        if (infolist.empty())
        {
            return true;
        }
        bool bRet = false;
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgs_slave");
        if (dbConn)
        {
            for (auto it = infolist.begin(); it != infolist.end(); )
            {
                MBCAF::Proto::MessageType type = it->msg_type();
                if ((MBCAF::Proto::MT_GroupAudio == type) || (MBCAF::Proto::MT_Audio == type))
                {
                    String strSql = "select * from MACAF_Audio where id=" + it->msg_data();
                    DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
                    if (resSet)
                    {
                        while (resSet->nextRow())
                        {
                            uint32_t dur;
                            uint32_t size;
                            String strPath;
                            resSet->getValue("duration", dur);
                            resSet->getValue("size", size);
                            resSet->getValue("path", strPath);
                            readAudioContent(dur, size, strPath, *it);
                        }
                        ++it;
                        delete resSet;
                    }
                    else
                    {
                        Mlog("no result for sql:%s", strSql.c_str());
                        it = infolist.erase(it);
                    }
                }
                else
                {
                    ++it;
                }
            }
            dbMag->freeTempConnect(dbConn);
            bRet = true;
        }
        else
        {
            Mlog("no connection for gsgsslave");
        }
        return bRet;
    }
    //-----------------------------------------------------------------------
    int Model_Audio::saveAudioInfo(uint32_t fromid, uint32_t toid, uint32_t ctime, const char* data, uint32_t dsize)
    {
        uint32_t dur;
        MemStream::read((Mui8 *)data, dur);
        Mui8 * tempdata = (Mui8 *)data + 4;
        uint32_t nRealLen = dsize - 4;
        int nAudioId = -1;

        CHttpClient httpClient;
        String strPath = httpClient.uploadFile(mFileURL, tempdata, nRealLen);
        if (!strPath.empty())
        {
            DatabaseManager * dbMag = M_Only(DatabaseManager);
            DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsmaster");
            if (dbConn)
            {
                uint32_t roft = 0;
                String strSql = "insert into MACAF_Audio(`fromId`, `toId`, `path`, `size`, `duration`, `created`) values(?, ?, ?, ?, ?, ?)";
                StrUtil::replace(strSql, fromid, '?', roft);
                StrUtil::replace(strSql, toid, '?', roft);
                StrUtil::replace(strSql, strPath, '?', roft);
                StrUtil::replace(strSql, nRealLen, '?', roft);
                StrUtil::replace(strSql, dur, '?', roft);
                StrUtil::replace(strSql, ctime, '?', roft);
                if (dbConn->exec(strSql.c_str()))
                {
                    nAudioId = dbConn->getInsertId();
                    Mlog("audioId=%d", nAudioId);
                }
                else
                {
                    Mlog("sql failed: %s", strSql.c_str());
                }
                dbMag->freeTempConnect(dbConn);
            }
            else
            {
                Mlog("no db connection for gsgsmaster");
            }
        }
        else
        {
            Mlog("upload file failed");
        }
        return nAudioId;
    }
    //-----------------------------------------------------------------------
    bool Model_Audio::readAudioContent(uint32_t dur, uint32_t size, const String & strPath, MBCAF::Proto::MsgInfo & cMsg)
    {
        if (strPath.empty() || dur == 0 || size == 0)
        {
            return false;
        }

        AudioMsgInfo audio;
        audio.data = new uchar_t[4 + size];
        MemStream::write(audio.data, dur);
        audio.data_len = 4;
        audio.fileSize = size;

        CHttpClient httpClient;
        if (!httpClient.downloadFile(strPath, &audio))
        {
            delete[] audio.data;
            return false;
        }

        Mlog("download_path=%s, data_len=%d", strPath.c_str(), audio.data_len);
        cMsg.set_msg_data((const char *)audio.data, audio.data_len);

        delete[] audio.data;
        return true;
    }
    //-----------------------------------------------------------------------
    CURLcode Model_Audio::Post(const String & url, const String & in, String & out)
    {
        CURLcode res;
        CURL * curl = curl_easy_init();
        if (NULL == curl)
        {
            return CURLE_FAILED_INIT;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, in.c_str());
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&out);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return res;
    }
    //-----------------------------------------------------------------------
    CURLcode Model_Audio::Get(const String & url, String & out)
    {
        CURLcode res;
        CURL * curl = curl_easy_init();
        if (NULL == curl)
        {
            return CURLE_FAILED_INIT;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&out);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return res;
    }
    //-----------------------------------------------------------------------
    String Model_Audio::uploadFile(const String & url, void * data, int size)
    {
        if (url.empty())
            return "";
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL * curl = curl_easy_init();
        if (!curl)
        {
            curl_global_cleanup();
            return "";
        }
        struct curl_slist * headerlist = NULL;
        headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data; boundary=WebKitFormBoundary8riBH6S4ZsoT69so");
        // what URL that receives this POST
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    //enable verbose for easier tracing
        String body = "--WebKitFormBoundary8riBH6S4ZsoT69so\r\nContent-Disposition: form-data; name=\"file\"; filename=\"1.audio\"\r\nContent-Type:image/jpg\r\n\r\n";
        body.append((char*)data, size);    // image buffer
        String str = "\r\n--WebKitFormBoundary8riBH6S4ZsoT69so--\r\n\r\n";
        body.append(str.c_str(), str.size());
        // post binary data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        // set the size of the postfields data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
        String strResp;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResp);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

        // Perform the request, res will get the return code
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        if (CURLE_OK != res)
        {
            Mlog("curl_easy_perform failed, res=%d", res);
            return "";
        }

        Json::Reader reader;
        Json::Value value;

        if (!reader.parse(strResp, value))
        {
            Mlog("json parse failed: %s", strResp.c_str());
            return "";
        }

        if (value["error_code"].isNull())
        {
            Mlog("no code in response %s", strResp.c_str());
            return "";
        }
        uint32_t nRet = value["error_code"].asUInt();
        if (nRet != 0)
        {
            Mlog("upload faile:%u", nRet);
            return "";
        }
        return value["url"].asString();
    }
    //-----------------------------------------------------------------------
    bool Model_Audio::downloadFile(const String & url, AudioMsgInfo * out)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        CURL * curl = curl_easy_init();
        if (!curl)
        {
            curl_global_cleanup();
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_binary);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
        CURLcode res = curl_easy_perform(curl);

        int retcode = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retcode);
        if (CURLE_OK != res || retcode != 200)
        {
            Mlog("curl_easy_perform failed, res=%d, ret=%u", res, retcode);
        }
        double len = 0;
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        if (len != out->fileSize)
        {
            return false;
        }
        return true;
    }
    //-----------------------------------------------------------------------
}