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

#include "MdfSocketClientPrc.h"
#include "MdfConnectManager.h"
#include "MdfThreadManager.h"
#include "MdfSocketServerPrc.h"
#include "MdfLogManager.h"
#include "MdfStrUtil.h"

#ifdef _DEBUG
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
#else
    #define INVOCATION_ENTER()
    #define INVOCATION_RETURN(retval)
#endif
namespace Mdf
{
	//-----------------------------------------------------------------------
	SocketClientPrc::SocketClientPrc():
        Parent(0, 0, ACE_Reactor::instance()),
		mStrategy(ACE_Reactor::instance(), this, ACE_Event_Handler::WRITE_MASK),
		mBase(0),
		mDebugMark(0),
        mAutoDestroy(true),
        mSpliteMessage(false)
	{
		bool nodelay = true;
		peer().enable(ACE_NONBLOCK);
		peer().set_option(IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
		mSendMark = mReceiveMark = M_Only(ConnectManager)->getTimeTick();
		M_Trace("SocketClientPrc::SocketClientPrc()");
	}
    //-----------------------------------------------------------------------
	SocketClientPrc::SocketClientPrc(ClientIO * base, ACE_Reactor * tor) :
		Parent(0, 0, tor ? tor : ACE_Reactor::instance()),
        mStrategy(tor ? tor : ACE_Reactor::instance(), this, ACE_Event_Handler::WRITE_MASK),
		mBase(base),
		mDebugMark(0),
        mAutoDestroy(true),
        mSpliteMessage(false)
    {
		bool nodelay = true;
		peer().enable(ACE_NONBLOCK);
		peer().set_option(IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
		mSendMark = mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        base->bind(this);
        M_Trace("SocketClientPrc:: SocketClientPrc(ClientIO *, ACE_Reactor *)");
    }
    //-----------------------------------------------------------------------
	SocketClientPrc::~SocketClientPrc()
    {
		M_Trace("SocketClientPrc::~SocketClientPrc()");
        if(mBase)
        {
            delete mBase;
			mBase = 0;
        }
    }
	//-----------------------------------------------------------------------
	SocketClientPrc * SocketClientPrc::createInstance(ACE_Reactor * tor) const
	{
        ClientIO * temp = mBase->createInstance();
		SocketClientPrc * re = new SocketClientPrc(temp, tor);
        temp->bind(re);
		return re;
	}
    //-----------------------------------------------------------------------
    void SocketClientPrc::setAutoDestroy(bool set)
    {
        mAutoDestroy = set;
    }
    //-----------------------------------------------------------------------
    bool SocketClientPrc::isAutoDestroy() const
    {
        return mAutoDestroy;
    }
	//-----------------------------------------------------------------------
	Mi32 SocketClientPrc::send(void * data, MCount cnt)
	{
		if(mBase->isStop())
			return 0;

		ACE_Message_Block * block = 0;
		ACE_NEW_RETURN(block, ACE_Message_Block(cnt), -1);
		block->copy((const char *)data, cnt);
        {
            ScopeLock tlock(mOutMute);
            putq(block);//autowakeupwrite
        }
		return cnt;
	}
	//-----------------------------------------------------------------------
	Mi32 SocketClientPrc::sendHead(void * data, MCount cnt)
	{
		if(mBase->isStop())
			return 0;

		ACE_Message_Block * block = 0;
		ACE_NEW_RETURN(block, ACE_Message_Block(cnt), -1);
		block->copy((const char *)data, cnt);
        {
            ScopeLock tlock(mOutMute);
            if(mSpliteMessage)
                putq(block);
            else
                ungetq(block);
        }
		return cnt;
	}
    //-----------------------------------------------------------------------
	int SocketClientPrc::open(void * p)
	{
		if(Parent::open(p) == -1)
			return -1;

		msg_queue()->notification_strategy(&mStrategy);

        ACE_INET_Addr tmpaddr;
        size_t addrsize = 1;
        ACE_SOCK_SEQPACK_Association ssa(peer().get_handle());

        ssa.get_local_addrs(&tmpaddr, addrsize); // local
#if _UNICODE
        mBase->setLocalIP(StrUtil::s2ws(tmpaddr.get_host_addr()), tmpaddr.get_port_number());
#else
        mBase->setLocalIP(tmpaddr.get_host_addr(), tmpaddr.get_port_number());
#endif

        addrsize = 1;
        ssa.get_remote_addrs(&tmpaddr, addrsize); // remote
#if _UNICODE
        mBase->setIP(StrUtil::s2ws(tmpaddr.get_host_addr()), tmpaddr.get_port_number());
#else
        mBase->setIP(tmpaddr.get_host_addr(), tmpaddr.get_port_number());
#endif
        mBase->mStop = false;
        M_Only(ConnectManager)->addClientConnect(mBase->getType(), mBase);
        M_Only(ConnectManager)->confirmClient(mBase->getType(), mBase->getIndex());
        mBase->onConfirm();
		return 0;
	}
	//-----------------------------------------------------------------------
	int SocketClientPrc::handle_input(ACE_HANDLE)
	{
		M_Trace("SocketClientPrc::handle_input(ACE_HANDLE)");

		INVOCATION_ENTER();

		while (1)
		{
			Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
			if (bufresever < M_SocketBlockSize)
				mReceiveBuffer.enlarge(M_SocketBlockSize);

			int recsize = peer().recv(mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);
			if (recsize <= 0)
			{
				if (ACE_OS::last_error() != EWOULDBLOCK)
				{
					INVOCATION_RETURN(-1);
				}
				else
				{
					break;
				}
			}

			mReceiveBuffer.writeSkip(recsize);

			mReceiveMark = M_Only(ConnectManager)->getTimeTick();
		}

		Message * msg = NULL;
		try
		{
			while ((msg = mBase->create(mReceiveBuffer.getBuffer(), mReceiveBuffer.getWriteSize())))
			{
				uint32_t msgsize = msg->getSize();

				mBase->onMessage(msg);

				mReceiveBuffer.readSkip(msgsize);
				mBase->destroy(msg);
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
	int SocketClientPrc::handle_timeout(const ACE_Time_Value &, const void *)
	{
		Mui32 temp = M_Only(ConnectManager)->getTimeTick();
		mBase->onTimer(temp);
		return 0;
	}
	//-----------------------------------------------------------------------
	int SocketClientPrc::handle_output(ACE_HANDLE)
	{
		M_Trace("SocketClientPrc::handle_output(ACE_HANDLE)");
		INVOCATION_ENTER();

    ContinueTransmission:
		ACE_Message_Block * mb = 0;
		//ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != getq(mb))
		{
            mOutMute.acquire();
            mSpliteMessage = true;
            mOutMute.release();
			ssize_t dsedsize = peer().send(mb->rd_ptr(), mb->length());
			if (dsedsize <= 0)
			{
				if (ACE_OS::last_error() != EWOULDBLOCK)
				{
                    mb->release();
					INVOCATION_RETURN(-1);
				}
				else
				{
                    ungetq(mb);
					break;
				}
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
            mOutMute.acquire();
            mSpliteMessage = false;
            mOutMute.release();
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
	int SocketClientPrc::handle_exception(ACE_HANDLE)
	{
		M_Trace("SocketClientPrc::handle_exception(ACE_HANDLE)");
        mBase->mAbort = true;
        mBase->mStop = true;
        mBase->onException();
		return -1;
	}
	//-----------------------------------------------------------------------
	/*int SocketClientPrc::handle_signal(int signum, siginfo_t *, ucontext_t *)
	{

	}*/
	//-----------------------------------------------------------------------
	int SocketClientPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
	{
		M_Trace("SocketClientPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)");
		//if (peer().close() == -1)
		//	MlogError(ACE_TEXT("%N:%l: Failed to close socket. ")
		//		ACE_TEXT("(errno = %i: %m)\n"), ACE_ERRNO_GET);
        mBase->mStop = true;
        mBase->setTimer(false, 0, 0);
        M_Only(ConnectManager)->removeClientConnect(mBase->getType(), mBase);
        M_Only(ConnectManager)->resetClient(mBase->getType(), mBase->getIndex());
        mBase->onClose();

        //reactor()->end_reactor_event_loop();
        //if (reactor()->remove_handler(this, READ_MASK | EXCEPT_MASK | DONT_CALL) == -1)
        //    MlogError(ACE_TEXT("%N:%l: Failed to register (errno = %i: %m)\n"), ACE_ERRNO_GET);
        if(mAutoDestroy)
		    delete this;
		return 0;
	}
	//-----------------------------------------------------------------------
}