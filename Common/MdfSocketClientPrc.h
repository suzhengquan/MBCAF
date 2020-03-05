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

#ifndef _MDF_SocketClientPrc_H_
#define _MDF_SocketClientPrc_H_

#include "MdfPreInclude.h"
#include "MdfRingBuffer.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class MdfNetAPI SocketClientPrc : public ACE_Svc_Handler<ACE_SOCK_Stream, ACE_MT_SYNCH>
    {
		typedef ACE_Svc_Handler<ACE_SOCK_Stream, ACE_MT_SYNCH> Parent;
    public:
        SocketClientPrc();
		SocketClientPrc(ClientIO * base, ACE_Reactor * tor);
        virtual ~SocketClientPrc();

		/**
		@version 0.9.1
		*/
		virtual SocketClientPrc * createInstance(ACE_Reactor * tor) const;

        /**
        @version 0.9.1
        */
        void setAutoDestroy(bool set);

        /**
        @version 0.9.1
        */
        bool isAutoDestroy() const;

		/**
		@version 0.9.1
		*/
		Mi32 send(void * data, MCount cnt);

		/**
		@version 0.9.1
		*/
		inline ConnectID getHandle() const
		{
			return peer().get_handle();
		}

		/**
		@version 0.9.1
		*/
		inline ClientIO * getBase() const
		{
			return mBase;
		}

		inline ACE_SOCK_Stream * getStream() const
		{
			return &peer();
		}

        /// @copydetails ACE_Svc_Handler::open
		virtual int open(void * = 0);

		/// @copydetails ACE_Svc_Handler::handle_input
		virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Svc_Handler::handle_output
		virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Svc_Handler::handle_timeout
		virtual int handle_timeout(const ACE_Time_Value & current_time, const void * act = 0);

        /// @copydetails ACE_Svc_Handler::handle_exception
		virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Svc_Handler::handle_signal
		//virtual int handle_signal(int signum, siginfo_t * = 0, ucontext_t * = 0);

        /// @copydetails ACE_Svc_Handler::handle_close
		virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);
    private:
		ClientIO * mBase;
		RingBuffer mReceiveBuffer;
		ACE_Reactor_Notification_Strategy mStrategy;
        ACE_Thread_Mutex mOutMute;
		Mui32 mSendMark;
		Mui32 mReceiveMark;
		int mDebugMark;
        bool mAutoDestroy;
    };
}

#endif