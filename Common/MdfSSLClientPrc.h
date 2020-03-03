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

#ifndef _MDF_SSLClientPrc_H_
#define _MDF_SSLClientPrc_H_

#include "MdfPreInclude.h"
#include "MdfRingBuffer.h"
#include "MdfSocketClientPrc.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class MdfNetAPI SSLClientPrc : public SocketClientPrc
    {
    public:
        SSLClientPrc();
		SSLClientPrc(ClientIO * base, ACE_Reactor * tor);
        virtual ~SSLClientPrc();

        /**
        @version 0.9.1
        */
        int init(const String & certfile = "", const String & keyfile = "", const String & keypw = “”);

        /**
        @version 0.9.1
        */
        inline SSL_CTX * getSSLCTX() const { return mSSLctx; }

        /**
        @version 0.9.1
        */
        inline SSL * getSSL() const { return mSSL; }

        /**
        @version 0.9.1
        */
        inline bool isSSLConnect() const { return mSSLConnect; }

        /**
        @version 0.9.1
        */
        const string & getCertFile() const { return mCertFile; }

        /**
        @version 0.9.1
        */
        const string & getKeyFile() const { return mKeyFile; }

        /**
        @version 0.9.1
        */
        const string & gKeyPassword() const { return mKeyPW; }

        /// @copydetails SocketServerPrc::createInstance
		virtual SSLClientPrc * createInstance() const;

        /// @copydetails ACE_Svc_Handler::open
        virtual int open(void * = 0);

		/// @copydetails ACE_Svc_Handler::handle_input
		virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Svc_Handler::handle_output
		virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Svc_Handler::handle_close
		virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
    protected:
        Mi32 connectSSL();
    private:
        SSL_CTX * mSSLctx;
        SSL * mSSL;
        String mCertFile;
        String mKeyFile;
        String mKeyPW;
        bool mSSLConnect;
    };
}

#endif