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

#include "MdfSocketServerPrc.h"
#include "MdfConnectManager.h"
#include "MdfLogManager.h"
#include "MdfThreadManager.h"
#include "MdfServerIO.h"
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
	SocketServerPrc::SocketServerPrc():
        mStrategy(0, this, ACE_Event_Handler::WRITE_MASK),
		mBase(0),
        mReactor(0),
        mDebugMark(0),
        mAutoDestroy(true),
        mSplitMessage(false)
    {
        mSendMark = mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        M_Trace("SocketServerPrc::SocketServerPrc()");
    }
    //-----------------------------------------------------------------------
	SocketServerPrc::SocketServerPrc(ServerIO * prc, ACE_Reactor * tor) :
        mReactor(tor ? tor : ACE_Reactor::instance()),
        mStrategy(0, this, ACE_Event_Handler::WRITE_MASK),
        mBase(prc),
        mDebugMark(0),
        mAutoDestroy(true),
        mSplitMessage(false)
    {
        mSendMark = mReceiveMark = M_Only(ConnectManager)->getTimeTick();
        prc->bind(this);
        M_Trace("SocketServerPrc::SocketServerPrc(ClientIO *, ACE_Reactor *)");
    }
    //-----------------------------------------------------------------------
	SocketServerPrc::~SocketServerPrc()
    {
        M_Trace("SocketServerPrc::~SocketServerPrc()");
        if(mBase)
        {
            delete mBase;
			mBase = 0;
        }
    }
	//-----------------------------------------------------------------------
	SocketServerPrc * SocketServerPrc::createInstance(ACE_Reactor * tor) const
	{
		ServerIO * temp = mBase->createInstance();
		SocketServerPrc * re = new SocketServerPrc(temp, tor);
		temp->bind(re);

		return re;
	}
    //-----------------------------------------------------------------------
    void SocketServerPrc::setAutoDestroy(bool set)
    {
        mAutoDestroy = set;
    }
    //-----------------------------------------------------------------------
    bool SocketServerPrc::isAutoDestroy() const
    {
        return mAutoDestroy;
    }
    //-----------------------------------------------------------------------
	Mi32 SocketServerPrc::send(void * data, MCount cnt)
    {
		if(mBase->isStop())
		{
			return 0;
		}

        ACE_Message_Block * block = 0;
        ACE_NEW_RETURN(block, ACE_Message_Block(cnt), -1);
        block->copy((const char *)data, cnt);
        {
            ScopeLock tlock(mOutMute);
            if (mOutQueue.enqueue_tail(block) == -1)//autowakeupwrite
            {
                MlogError(ACE_TEXT("(%P|%t) %p; discarding data\n"), ACE_TEXT("enqueue failed"));
                block->release();
                return 0;
            }
        }
        return cnt;
    }
    //-----------------------------------------------------------------------
	Mi32 SocketServerPrc::sendHead(void * data, MCount cnt)
    {
		if(mBase->isStop())
		{
			return 0;
		}

        ACE_Message_Block * block = 0;
        ACE_NEW_RETURN(block, ACE_Message_Block((const char *)data, cnt), -1);
        block->copy((const char *)data, cnt);
        {
            ScopeLock tlock(mOutMute);
            if(mSplitMessage)
            {
                ACE_Message_Block * mb = 0;
                if(0 <= mOutQueue.peek_dequeue_head(mb))
                {
                    if(mb->cont() == 0)
                    {
                        mb->cont(block);
                        return cnt;
                    }
                }

                if (mOutQueue.enqueue_tail(block) == -1)
                {
                    MlogError(ACE_TEXT("(%P|%t) %p; discarding data\n"), ACE_TEXT("enqueue failed"));
                    block->release();
                    return 0;
                }
            }
            else
            {
                if (mOutQueue.enqueue_head(block) == -1)
                {
                    MlogError(ACE_TEXT("(%P|%t) %p; discarding data\n"), ACE_TEXT("enqueue failed"));
                    block->release();
                    return 0;
                }
            }
        }
        return cnt;
    }
    //-----------------------------------------------------------------------
	ServerIO * SocketServerPrc::getBase() const
    {
        return mBase;
    }
	//-----------------------------------------------------------------------
	ACE_HANDLE SocketServerPrc::getHandle() const
	{
		return mBase->getStream()->get_handle();
	}
    //-----------------------------------------------------------------------
    int SocketServerPrc::handle_connect()
    {
        if (mReactor->register_handler(this, READ_MASK | EXCEPT_MASK) == -1)
            MlogErrorReturn(logError(ACE_TEXT("%N:%l: Failed to register ")
                ACE_TEXT("read handler. (errno = %i: %m)\n"), ACE_ERRNO_GET), -1);

        mStrategy.reactor(mReactor);
        mOutQueue.notification_strategy(&mStrategy);

        bool nodelay = true;
        mBase->getStream()->enable(ACE_NONBLOCK);
        mBase->getStream()->set_option(IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

        ACE_INET_Addr tmpaddr;
        size_t addrsize = 1;
        ACE_SOCK_SEQPACK_Association ssa(mBase->getStream()->get_handle());

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
        M_Only(ConnectManager)->addServerConnect(mBase->getType(), mBase);
        mBase->onConnect();

        return 0;
    }
    //-----------------------------------------------------------------------
	int SocketServerPrc::handle_input(ACE_HANDLE)
    {
        M_Trace("SocketServerPrc::handle_input(ACE_HANDLE)");

        INVOCATION_ENTER();

        while(1)
        {
            Mui32 bufresever = mReceiveBuffer.getAllocSize() - mReceiveBuffer.getWriteSize();
            if(bufresever < M_SocketBlockSize)
                mReceiveBuffer.enlarge(M_SocketBlockSize);

            int recsize = mBase->getStream()->recv(mReceiveBuffer.getBuffer() + mReceiveBuffer.getWriteSize(), M_SocketBlockSize);
			if (recsize <= 0)
			{
				if (ACE_OS::last_error() != EWOULDBLOCK)
				{
					INVOCATION_RETURN(-1);
				}
				else
					break;
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
            if(msg)
            {
                delete msg;
                msg = NULL;
            }
            INVOCATION_RETURN(-1);
        }
        INVOCATION_RETURN(0);
    }
    //-----------------------------------------------------------------------
	int SocketServerPrc::handle_output(ACE_HANDLE)
    {
        M_Trace("SocketServerPrc::handle_output(ACE_HANDLE)");
		INVOCATION_ENTER();

    ContinueTransmission:
        ACE_Message_Block * mb = 0;
        //ACE_Time_Value nowait(ACE_OS::gettimeofday());
        while (0 <= mOutQueue.peek_dequeue_head(mb))
        {
            int sedsize = mb->length();
            if(sedsize > M_SocketOutSize)
            {
                sedsize = M_SocketOutSize;
            }

            mOutMute.acquire();
            mSplitMessage = true;
            mOutMute.release();
            int dsedsize = mBase->getStream()->send(mb->rd_ptr(), sedsize);
            if(dsedsize <= 0)
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
            else
            {
                mb->rd_ptr((size_t)dsedsize);
            }
            mSendMark = M_Only(ConnectManager)->getTimeTick();
            
            if (mb->length() <= 0)
            {
                ScopeLock tlock(mOutMute);
                mSplitMessage = false;
                ACE_Message_Block * cnt = mb->cont();
                if(cnt)
                {
                    mb->cont(0);
                    mOutQueue.dequeue_head(mb);
                    mb->release();
                    if (mOutQueue.enqueue_head(cnt) == -1)
                    {
                        cnt->release();
                        INVOCATION_RETURN(-1);
                    }
                }
                else
                {
                    mOutQueue.dequeue_head(mb);
                    mb->release();
                }
            }
            else
            {
                break;
            }
        }

        if (mBase->isAbort())
        {
            INVOCATION_RETURN(-1);
        }
		else if (mBase->isStop())
		{
			if (!mOutQueue.is_empty())
			{
				ACE_OS::sleep(1);
				goto ContinueTransmission;
			}
			INVOCATION_RETURN(-1);
		}
        {
            ScopeLock tlock(mOutMute);
            if (mOutQueue.is_empty())
                mReactor->cancel_wakeup(this, ACE_Event_Handler::WRITE_MASK);
            else
                mReactor->schedule_wakeup(this, ACE_Event_Handler::WRITE_MASK);
        }
		INVOCATION_RETURN(0);
    }
	//-----------------------------------------------------------------------
	int SocketServerPrc::handle_timeout(const ACE_Time_Value &, const void *)
	{
		Mui32 temp = M_Only(ConnectManager)->getTimeTick();
		mBase->onTimer(temp);
		return 0;
	}
    //-----------------------------------------------------------------------
	int SocketServerPrc::handle_exception(ACE_HANDLE)
    {
        M_Trace("SocketServerPrc::handle_exception(ACE_HANDLE)");
        mBase->mAbort = true;
        mBase->mStop = true;
        mBase->onException();
        return -1;
    }
    //-----------------------------------------------------------------------
	int SocketServerPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
    {
        M_Trace("SocketServerPrc::handle_close(ACE_HANDLE, ACE_Reactor_Mask)");
        M_Only(ConnectManager)->removeServerConnect(mBase->getType(), mBase);
        mBase->mStop = true;
        mBase->setTimer(false, 0, 0);
        
        mBase->onClose();

        if (mBase->getStream()->close() == -1)
            MlogError(ACE_TEXT("%N:%l: Failed to close socket. (errno = %i: %m)\n"), ACE_ERRNO_GET);

        mOutQueue.flush();

        if (mReactor->remove_handler(this, READ_MASK | EXCEPT_MASK | DONT_CALL) == -1)
            MlogError(ACE_TEXT("%N:%l: Failed to register (errno = %i: %m)\n"), ACE_ERRNO_GET);

        if (mAutoDestroy)
            delete this;
        return 0;
    }
    //-----------------------------------------------------------------------
}