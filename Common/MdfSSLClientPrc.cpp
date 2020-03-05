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

#include "MdfSSLClientPrc.h"
#include "MdfConnectManager.h"
#include "MdfThreadManager.h"
#include "MdfSocketServerPrc.h"
#include "MdfLogManager.h"
#include "MdfStrUtil.h"

/**
 * This macro is used to increase the invocation counter by one when entering
 * handle_input(). It also checks wether the counter is greater than zero
 * indicating, that handle_input() has been called before.
*/
#define INVOCATION_ENTER()										\
{																\
    if (mDebugMark > 0)											\
        MlogError(ACE_TEXT("Multiple invocations detected.\n"));\
    mDebugMark++;												\
}

/*
 * THis macro is the counter part to INVOCATION_ENTER(). It decreases the
 * invocation counter and then returns the given value. This macro is
 * here for convenience to decrease the invocation counter also when returning
 * due to errors.
*/
#define INVOCATION_RETURN(retval) { mDebugMark--; return retval; }

namespace Mdf
{
	//-----------------------------------------------------------------------
	SSLClientPrc::SSLClientPrc()
	{
        mSSLctx = NULL;
        mSSL = NULL;
        mSSLConnect = false;
		M_Trace("SSLClientPrc::SSLClientPrc()");
	}
    //-----------------------------------------------------------------------
	SSLClientPrc::SSLClientPrc(ClientIO * base, ACE_Reactor * tor) :
        SocketClientPrc(base, tor)
    {
        mSSLctx = NULL;
        mSSL = NULL;
        mSSLConnect = false;
        M_Trace("SSLClientPrc:: SSLClientPrc(ClientIO *, ACE_Reactor *)");
    }
    //-----------------------------------------------------------------------
	SSLClientPrc::~SSLClientPrc()
    {
		M_Trace("SSLClientPrc::~SSLClientPrc()");
    }
    //-----------------------------------------------------------------------
    int SSLClientPrc::init(const String & certfile, const String & keyfile, const String & keypw)
    {
        if(mSSLctx)
            return 0;
        mCertFile = certfile;
        mKeyFile = keyfile;
        mKeyPW = keypw;

        SSLeay_add_ssl_algorithms();
        
        mSSLctx = SSL_CTX_new(SSLv23_client_method());
        if (mSSLctx)
        {
            if (mCertFile.size())
            {
                if (!SSL_CTX_use_certificate_file(mSSLctx, mCertFile.c_str(), SSL_FILETYPE_PEM))
                {
                    MlogError("init ssl: use certificate file failed.");
                    SSL_CTX_free(mSSLctx);
                    mSSLctx = 0;
                    return -1;
                }
            }

            SSL_CTX_set_default_passwd_cb_userdata(mSSLctx, (void*)keypw.c_str());

            if (mKeyFile.size())
            {
                if (!SSL_CTX_use_PrivateKey_file(mSSLctx, mKeyFile.c_str(), SSL_FILETYPE_PEM))
                {
                    MlogError("init ssl: use private key file failed.");
                    SSL_CTX_free(mSSLctx);
                    mSSLctx = 0;
                    return -1;
                }
            }

            if (!SSL_CTX_check_private_key(mSSLctx))
            {
                MlogError("init ssl: check private key file failed.");
                SSL_CTX_free(mSSLctx);
                mSSLctx = 0;
                return -1;
            }

            if (NULL == mSSL)
            {
                mSSL = SSL_new(mSSLctx);
                if (NULL == mSSL)
                {
                    MlogError("init ssl, create SSL object failed.");
                    return -1;
                }
            }
        }
        else
        {
            MlogError("init ssl: create new SSL_CTX object failed.");
            return -1;
        }
        return 0;
    }
	//-----------------------------------------------------------------------
	SSLClientPrc * SSLClientPrc::createInstance(ACE_Reactor * tor) const
	{
        ClientIO * temp = mBase->createInstance();
		SSLClientPrc * re = new SSLClientPrc(temp, tor);
        temp->bind(re);
		return re;
	}
    //-----------------------------------------------------------------------
    int SSLClientPrc::open(void * p)
    {
        SSL_set_mode(mSSL, SSL_MODE_AUTO_RETRY);
        if (SSL_set_fd(mSSL, peer().get_handle()) != 1)
        {
            MlogError("ssl set fd failed");
            return -1;
        }
        if(connectSSL() == -1)
            return -1;

        return SocketClientPrc::open(p);
    }
	//-----------------------------------------------------------------------
	int SSLClientPrc::handle_input(ACE_HANDLE)
	{
		M_Trace("SSLClientPrc::handle_input(ACE_HANDLE)");

		INVOCATION_ENTER();

        if (mSSLConnect)
        {
            while (1)
            {
                Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
                if (bufresever < M_SocketBlockSize)
                    mReceiveBuffer.enlarge(M_SocketBlockSize);

                int recsize = SSL_read(mSSL, mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);

                if (recsize == 0)
                {
                    Mi32 eCode = SSL_get_error(mSSL, recsize);
                    if (SSL_ERROR_ZERO_RETURN == eCode)
                        MlogWarning("recv ssl data error, peer closed.");
                    else
                        MlogError("recv ssl data error.");
                    INVOCATION_RETURN(-1);
                }
                else if (recsize < 0)
                {
                    Mi32 eCode = SSL_get_error(mSSL, recsize);
                    if (SSL_ERROR_WANT_READ == eCode || SSL_ERROR_WANT_WRITE == eCode)
                    {
                        break;
                    }
                    else
                    {
                        MlogError("recv ssl data error, errno: %d.", eCode);
                        INVOCATION_RETURN(-1);
                    }
                }

                mReceiveBuffer.writeSkip(recsize);

                mReceiveMark = M_Only(ConnectManager)->getTimeTick();
            }
        }
        else
        {
            INVOCATION_RETURN(connectSSL());
        }
		Message * msg = NULL;
		try
		{
			while ((msg = mBase->create(mReceiveBuffer.getBuffer(), mReceiveBuffer.getWriteSize())))
			{
				uint32_t msgsize = msg->getSize();

				mBase->onMessage(msg);

				mReceiveBuffer.readSkip(msgsize);
				delete msg;
				msg = NULL;
			}
		}
		catch (...)
		{
			if (msg)
			{
				mBase->onReceiveError();
				delete msg;
				msg = NULL;
			}
			INVOCATION_RETURN(-1);
		}
		INVOCATION_RETURN(0);
	}
	//-----------------------------------------------------------------------
	int SSLClientPrc::handle_output(ACE_HANDLE)
	{
		M_Trace("SSLClientPrc::handle_output(ACE_HANDLE)");
		INVOCATION_ENTER();

    ContinueTransmission:
		ACE_Message_Block * mb = 0;
		//ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != getq(mb))
		{
			ssize_t dsedsize = SSL_write(mSSL, mb->rd_ptr(), mb->length());
            if (dsedsize < 0)
            {
                Mi32 ecode = SSL_get_error(mSSL, dsedsize);
                if (SSL_ERROR_WANT_WRITE == ecode || SSL_ERROR_WANT_READ == ecode)
                {
                    ungetq(mb);
                    break;
                }
                else
                {
                    mb->release();
                    MlogError("send ssl data error, errno: %d.", ecode);
                    INVOCATION_RETURN(-1);
                }
            }
            else if (dsedsize == 0)
            {
                Mi32 ecode = SSL_get_error(mSSL, dsedsize);
                if (SSL_ERROR_ZERO_RETURN == ecode)
                {
                    MlogWarning("send ssl data error, peer closed.");
                }
                else
                {
                    MlogError("send ssl data error, errno: %d.", ecode);
                }
                mb->release();
                INVOCATION_RETURN(-1);
            }
			else
			{
                mb->rd_ptr((size_t)dsedsize);
			}
			if (mb->length() > 0)
			{
				ungetq(mb);
				break;
			}
            mb->release();
            mSendMark = M_Only(ConnectManager)->getTimeTick();
		}

        if (mBase->isAbort())
        {
            INVOCATION_RETURN(-1);
        }
        else if (mBase->isStop())
        {
            if (!msg_queue()->is_empty())
            {
                ACE_OS::sleep(1);
                goto ContinueTransmission;
            }
            INVOCATION_RETURN(-1);
        }
        {
            ScopeLock tlock(mOutMute);
            if (msg_queue()->is_empty())
                reactor()->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
            else
                reactor()->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
        }
		INVOCATION_RETURN(0);
	}
	//-----------------------------------------------------------------------
	int SSLClientPrc::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask rmask)
	{
		M_Trace("SSLClientPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)");
		//if (peer().close() == -1)
		//	MlogError(ACE_TEXT("%N:%l: Failed to close socket. ")
		//		ACE_TEXT("(errno = %i: %m)\n"), ACE_ERRNO_GET);

        if (mSSL)
        {
            Mi32 re = SSL_shutdown(mSSL);
            if (re <= 0)
            {
                Mi32 nErrorCode = SSL_get_error(mSSL, re);
                MlogWarning("ssl shutdown not finished, errno: %d.", nErrorCode);
            }
            else if (re == 1)
            {
                Mlog("ssl shutdown successed.");
            }
            SSL_free(mSSL);
            mSSL = NULL;
        }
        if (mSSLctx)
        {
            SSL_CTX_free(mSSLctx);
            mSSLctx = NULL;
        }
        mSSLConnect = false;

		return SocketClientPrc::handle_close(handle, rmask);
	}
    //-----------------------------------------------------------------------
    Mi32 SSLClientPrc::connectSSL()
    {
        Mi32 re = SSL_connect(mSSL);
        if (re == 1)
        {
            Mlog("ssl connect successed, remote ip: %s, port: %d.", GetRemoteIP(), GetRemotePort());
            mSSLConnect = true;
        }
        else if (re == 0)
        {
            Mi32 ecode = SSL_get_error(mSSL, re);
            MlogError("ssl connect was shut down, remote ip: %s, port: %d, error code: %d.",
                GetRemoteIP(), GetRemotePort(), ecode);
            return -1;
        }
        else
        {
            Mi32 ecode = SSL_get_error(mSSL, re);
            if (SSL_ERROR_WANT_READ == ecode || SSL_ERROR_WANT_WRITE == ecode)
            {//try again
                MlogWarning("ssl connect is blocking, remote ip: %s, port: %d, error code: %d.",
                    GetRemoteIP(), GetRemotePort(), ecode);
            }
            else
            {
                MlogError("ssl connect failed, remote ip: %s, port: %d, error code: %d.",
                    GetRemoteIP(), GetRemotePort(), ecode);
                return -1;
            }
        }
        return 0;
    }
	//-----------------------------------------------------------------------
}