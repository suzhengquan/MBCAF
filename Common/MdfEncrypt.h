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

#ifndef _MDF_Encrypt_
#define _MDF_Encrypt_

#include "MdfPreInclude.h"
#include <iostream>
#include <openssl/aes.h>
#include <openssl/md5.h>

using namespace std;

namespace Mdf
{
    String base64_decode(const String & ascdata);
    String base64_encode(const String & bindata);

    class AES
    {
    public:
        AES(const String & strKey);

        int Encrypt(const char * in, uint32_t insize, char ** outdata, uint32_t& outsize);
        int Decrypt(const char * in, uint32_t insize, char ** outdata, uint32_t& outsize);
        void Free(char * data);

    private:
        AES_KEY m_cEncKey;
        AES_KEY m_cDecKey;
    };

    class MD5
    {
    public:
        static void calc(const char* pContent, unsigned int nLen, char* md5);
    };

	/**
	@version 0.9.1
	*/
	Mui32 calcHash(const char * str)
	{
		register Mui32 hash = 0;
		while (Mui32 ch = (Mui32)*str++)
		{
			hash = hash * 131 + ch;       
		}
		return hash;
	}

    /**
    @version 0.9.1
	*/
    int genToken(unsigned int uid, time_t time_offset, char * md5_str_buf);

    /**
    @version 0.9.1
    */
    bool IsTokenValid(uint32_t user_id, const char * token);
}

#endif