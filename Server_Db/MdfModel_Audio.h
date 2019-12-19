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

#ifndef _MDF_MODEL_AUDIO_H_
#define _MDF_MODEL_AUDIO_H_

#include "MdfPreInclude.h"
#include "MdfStrUtil.h"
#include "MBCAF.Proto.pb.h"
#include <curl/curl.h>

namespace Mdf
{
    typedef struct AudioMsgInfo_t
    {
        uint32_t audioId;
        uint32_t fileSize;
        uint32_t data_len;
        Mui8 * data;
        String path;
    } AudioMsgInfo;

    class Model_Audio
    {
    public:
        Model_Audio();
        virtual ~Model_Audio();

        /** 
        @version 0.9.1
        */
        void setUrl(String & url);

        /**
        @version 0.9.1
         */
        bool readAudios(list<MBCAF::Proto::MsgInfo> & info);

        /** 
        @version 0.9.1
        */
        int saveAudioInfo(uint32_t fromid, uint32_t toid, uint32_t time, const char * data, uint32_t dur);

        /**
        @version 
        */
        bool readAudioContent(uint32_t dur, uint32_t size, const String & path, MBCAF::Proto::MsgInfo & msg);

        /**
        @version
        */
        static CURLcode Post(const String & strUrl, const String & in, String & out);

        /**
        @version
        */
        static CURLcode Get(const String & strUrl, String & out);

        /**
        @version
        */
        static String uploadFile(const String & url, void * data, int size);

        /**
        @version
        */
        static bool downloadFile(const String & url, AudioMsgInfo * out);
    private:
        String mFileURL;
    };

    M_SingletonImpl(Model_Audio);
}

#endif
