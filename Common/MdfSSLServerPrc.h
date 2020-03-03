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

#ifndef _MDF_SSLServerPrc_H_
#define _MDF_SSLServerPrc_H_

#include "MdfPreInclude.h"
#include "MdfSocketServerPrc.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

namespace Mdf
{
    /** SSL IO¥¶¿Ì
    @version 0.9.1
    */
    class SSLServerPrc : public SocketServerPrc
    {
        friend class SSLAcceptPrc;
    public:
        SSLServerPrc(ServerIO * prc, ACE_Reactor * tor);
        virtual ~SSLServerPrc();

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

		/// @copydetails SocketServerPrc::createInstance
		virtual SSLServerPrc * createInstance(ACE_Reactor * tor) const;

        /// @copydetails SocketServerPrc::handle_connect
        virtual int handle_connect();

        /// @copydetails ACE_Event_Handler::handle_input
        virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Event_Handler::handle_output
        virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Event_Handler::handle_close
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);
    protected:
        SSLServerPrc();
        
        Mi32 connectSSL();
        
        /**
        @version 0.9.1 avd
        */
        inline void setSSLCTX(SSL_CTX * ctx) { mSSLctx = ctx; }
    protected:
        SSL * mSSL;
        SSL_CTX * mSSLctx;
        bool mSSLConnect;
    };
}
#endif