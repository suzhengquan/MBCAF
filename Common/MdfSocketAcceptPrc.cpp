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

#include "MdfSocketAcceptPrc.h"
#include "MdfLogManager.h"
#include "MdfServerIO.h"

namespace Mdf
{
    //-----------------------------------------------------------------------
    SocketAcceptPrc::SocketAcceptPrc():
        mReactor(0), 
        mPattern(0)
    {
    }
    //-----------------------------------------------------------------------
	SocketAcceptPrc::SocketAcceptPrc(SocketServerPrc * ioPrc,
		const String & ip, Mui16 port, ACE_Reactor * tor) :
        ACE_Event_Handler(),
        mReactor(tor ? tor : ACE_Reactor::instance()),
        mPattern(ioPrc),
		mIP(ip),
		mPort(port)
    {
        M_Trace("SocketAcceptPrc:: SocketAcceptPrc(ACE_Reactor *)");
    }
    //-----------------------------------------------------------------------
	SocketAcceptPrc::~SocketAcceptPrc()
    {
		if (mPattern)
		{
			delete mPattern;
			mPattern = 0;
		}
        M_Trace("SocketAcceptPrc::~SocketAcceptPrc()");
    }
    //-----------------------------------------------------------------------
	int SocketAcceptPrc::open()
    {
        M_Trace("SocketAcceptPrc::open(void)");

        ACE_INET_Addr hostAddr(mPort, mIP.c_str());

		mAcceptor.enable(ACE_NONBLOCK);

        if(mAcceptor.open(hostAddr, 1) == -1)
            MlogErrorReturn(logError(ACE_TEXT("%N:%l: Failed to open ")
				ACE_TEXT("listening socket. (errno = %i: %m)\n"), ACE_ERRNO_GET), -1);

        if(mReactor->register_handler(this, ACCEPT_MASK) == -1)
        {
			MlogError(ACE_TEXT("%N:%l: Failed to register accept handler. (errno = %i: %m)\n"), ACE_ERRNO_GET);

            if(mAcceptor.close() == -1)
				MlogError(ACE_TEXT("%N:%l: Failed to close the socket ")
                 ACE_TEXT("after previous error. (errno = %i: %m)\n"), ACE_ERRNO_GET);

            return -1;
        }

        return 0;
    }
    //-----------------------------------------------------------------------
	ACE_HANDLE SocketAcceptPrc::get_handle() const
    {
        return mAcceptor.get_handle();
    }
    //-----------------------------------------------------------------------
	int SocketAcceptPrc::handle_input(ACE_HANDLE)
    {
        M_Trace("SocketAcceptPrc::handle_input(ACE_HANDLE)");

        ACE_INET_Addr clientAddr;
		SocketServerPrc * temp = mPattern->createInstance(mReactor);
		ServerIO * base = temp->getBase();
        if(mAcceptor.accept(*base->getStream(), &clientAddr) == -1)
			MlogErrorReturn(logError(ACE_TEXT("%N:%l: Failed to accept ")
				ACE_TEXT("client connection. (errno = %i: %m)\n"), ACE_ERRNO_GET), -1);

        if (temp->handle_connect() == -1)
            base->stop();
		return 0;
    }
	//-----------------------------------------------------------------------
	int SocketAcceptPrc::handle_signal(int, siginfo_t *, ucontext_t *)
	{
		M_Trace("SocketAcceptPrc::handle_signal(ACE_HANDLE)");
		return 0;
	}
    //-----------------------------------------------------------------------
	int SocketAcceptPrc::handle_exception(ACE_HANDLE)
    {
        M_Trace("SocketAcceptPrc::handle_exception(ACE_HANDLE)");
        return -1;
    }
    //-----------------------------------------------------------------------
	int SocketAcceptPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
    {
        M_Trace("SocketAcceptPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)");

        if (mAcceptor.get_handle() != ACE_INVALID_HANDLE)
        {
            mReactor->remove_handler(this, ACE_Event_Handler::ACCEPT_MASK | ACE_Event_Handler::DONT_CALL);
            //mReactor->end_reactor_event_loop();
            if (mAcceptor.close() == -1)
                MlogError(ACE_TEXT("%N:%l: Failed to close the socket. (errno = %i: %m)\n"), ACE_ERRNO_GET);
        }

        delete this;
        return 0;
    }
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	// writePid
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	void writePid()
	{
		Mui32 curPid;
#ifdef _WIN32
		curPid = (Mui32)GetCurrentProcess();
#else
		curPid = (Mui32)getpid();
#endif
		FILE * f = fopen("server.pid", "w");
		assert(f);
		char szPid[32];
		snprintf(szPid, sizeof(szPid), "%d", curPid);
		fwrite(szPid, strlen(szPid), 1, f);
		fclose(f);
	}
    //-----------------------------------------------------------------------
}