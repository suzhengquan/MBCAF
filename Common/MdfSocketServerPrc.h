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

#ifndef _MDF_SocketServerPrc_H_
#define _MDF_SocketServerPrc_H_

#include "MdfPreInclude.h"
#include "MdfRingBuffer.h"
#include "MdfMessage.h"

namespace Mdf
{
    /** IO¥¶¿Ì
    @version 0.9.1
    */
    class SocketServerPrc : public ACE_Event_Handler
    {
        friend class SocketAcceptPrc;
    public:
        SocketServerPrc(ServerIO * prc, ACE_Reactor * tor);
        virtual ~SocketServerPrc();

		/**
		@version 0.9.1
		*/
		virtual SocketServerPrc * createInstance(ACE_Reactor * tor) const;

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
        Mi32 sendHead(void * data, MCount cnt);

        /**
        @version 0.9.1
        */
		ServerIO * getBase() const;

		/**
		@version 0.9.1
		*/
		ACE_HANDLE getHandle() const;

		/** lastest send time(tick)
		@version 0.9.1
		*/
		inline Mui64 getSendMark() const { return mSendMark; }

		/** lastest receive time(tick)
		@version 0.9.1
		*/
		inline Mui64 getReceiveMark() const { return mReceiveMark; }

        /**
        @version 0.9.1
        */
        virtual int handle_connect();

        /// @copydetails ACE_Event_Handler::handle_input
        virtual int handle_input(ACE_HANDLE = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Event_Handler::handle_output
        virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

		/// @copydetails ACE_Event_Handler::handle_timeout
		virtual int handle_timeout(const ACE_Time_Value &, const void *);

        /// @copydetails ACE_Event_Handler::handle_exception
        virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

        /// @copydetails ACE_Event_Handler::handle_close
        virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);
    protected:
        SocketServerPrc();
    protected:
        ACE_Reactor * mReactor;
		ServerIO * mBase;
        ACE_Reactor_Notification_Strategy mStrategy;
        ACE_Message_Queue<ACE_MT_SYNCH> mOutQueue;
        ACE_Thread_Mutex mOutMute;
        RingBuffer mReceiveBuffer;
        Mui64 mSendMark;
        Mui64 mReceiveMark;
    #ifdef _DEBUG
        int mDebugMark;
    #endif
        bool mAutoDestroy;
        volatile bool mSplitMessage;
    };
}
#endif