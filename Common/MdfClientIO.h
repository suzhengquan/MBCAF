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

#ifndef _MDF_ClientIO_H_
#define _MDF_ClientIO_H_

#include "MdfPreInclude.h"
#include "MdfSocketClientPrc.h"
#include "MdfMessage.h"

namespace Mdf
{
    typedef void(ClientIO::*CPrcCB)(Message * msg);

    /** Á¬½Ó¶Ë(connect peer)
    @version 0.9.1
    */
    class MdfNetAPI ClientIO
    {
        friend class SocketClientPrc;
    public:
        ClientIO(ACE_Reactor * tor);
        virtual ~ClientIO();

        /**
        @version 0.9.1
        */
        virtual ClientIO * createInstance() const;

        /**
        @warning Call it first
        @version 0.9.1 adv api
        */
        void bind(SocketClientPrc * prc);

        /**
        @version 0.9.1 adv api
        */
        void setIP(const String & ip, Mui16 port);
        
        /**
        @version 0.9.1
        */
        const String & getIP() const;

        /**
        @version 0.9.1
        */
        Mui16 getPort() const;

        /**
        @version 0.9.1 adv api
        */
        void setLocalIP(const String & ip, Mui16 port);

        /**
        @version 0.9.1
        */
        const String & getLocalIP() const;

        /**
        @version 0.9.1
        */
        Mui16 getLocalPort() const;

        /**
        @version 0.9.1
        */
        inline ACE_SOCK_Stream * getStream() const
        {
            return &mIOPrc->peer();
        }

        /**
        @version 0.9.1
        */
        inline ConnectID getID() const 
        {
            return mIOPrc->peer().get_handle();
        }
        
        /**
        @version 0.9.1
        */
        inline Mi32 send(Message * msg) 
        { 
            return mIOPrc->send(msg->getBuffer(), msg->getSize());
        }

        /**
        @version 0.9.1
        */
        virtual Message * create(Mui8 * buf, Mui32 len);

        /**
        @version 0.9.1
        */
        inline void abort()
        {
            mAbort = true;
            mStop = true;
        }

        /**
        @version 0.9.1
        */
        inline bool isAbort() const
        {
            return mAbort.value();
        }

        /**
        @version 0.9.1
        */
        inline void stop()
        {
            mStop = true;
        }

        /**
        @version 0.9.1
        */
        inline bool isStop() const
        {
            return mStop.value();
        }

        /**
        @version 0.9.1
        */
        void setSendSize(Mi32 size);

        /**
        @version 0.9.1
        */
        Mi32 getSendSize() const;

        /**
        @version 0.9.1
        */
        void setRecvSize(Mi32 size);

        /**
        @version 0.9.1
        */
        Mi32 getRecvSize() const;

        /**
        @version 0.9.1
        */
        void setTTL(Mi32 ttl);

        /**
        @version 0.9.1
        */
        Mi32 getTTL() const;

        /**
        @version 0.9.1
        */
        void setTOS(Mi32 tos);

        /**
        @version 0.9.1
        */
        Mi32 getTOS() const;

        /**
        @version 0.9.1
        */
        void setSendTimeOut(Mi32 time);

        /**
        @version 0.9.1
        */
        void setRecvTimeOut(Mi32 time);

        /**
        @version 0.9.1
        */
        void setLinger(bool set, Mi32 time);

        /**
        @version 0.9.1
        */
        void setNoDelay(bool set);

        /**
        @version 0.9.1
        */
        inline Mi32 send(Mui8 tid, Mui8 cid, const google::protobuf::MessageLite * proto)
        {
            MdfMessage msg;
            msg.setProto(proto);
            msg.setCommandID(M_MSG(tid, cid));

            return mIOPrc->send(msg.getBuffer(), msg.getSize());
        }

        /**
        @version 0.9.1
        */
        inline Mi32 send(Mui8 tid, Mui8 cid, Mui16 seq_num, const google::protobuf::MessageLite * proto)
        {
            MdfMessage msg;
            msg.setProto(proto);
            msg.setCommandID(M_MSG(tid, cid));
            msg.setSeqIdx(seq_num);

            return mIOPrc->send(msg.getBuffer(), msg.getSize());
        }

        /**
        @version 0.9.1
        */
        inline Mi32 send(void * data, MCount cnt)
        {
            return mIOPrc->send(data, cnt);
        }

        /**
        @version 0.9.1
        */
        void setTimer(bool set, TimeDurMS delay, TimeDurMS interval);

        /**
        @version 0.9.1
        */
        void setMsgBaseIndex(Mi32 idx);

        /**
        @version 0.9.1
        */
        void setMsgQuestionPrc(Mui16 mid, CPrcCB prc);

        /**
        @version 0.9.1
        */
        void setMsgAnswerPrc(Mui16 mid, CPrcCB prc);

        /**
        @version 0.9.1
        */
        void setMsgQandAPrc(Mui16 mid, CPrcCB qprc, CPrcCB aprc);

        /**
        @version 0.9.1
        */
        void unMsgQuestionPrc(Mui16 mid);

        /**
        @version 0.9.1
        */
        void unMsgAnswerPrc(Mui16 mid);

        /**
        @version 0.9.1
        */
        void unMsgQandAPrc(Mui16 mid);

        /**
        @version 0.9.1
        */
        virtual void onConfirm();

        /**
        @version 0.9.1
        */
        virtual void onClose();

        /**
        @version 0.9.1
        */
        virtual void onReceiveError();

        /**
        @version 0.9.1
        */
        virtual void onTimer(TimeDurMS time);

        /**
        @version 0.9.1
        */
        virtual void onMessage(Message * msg);

        /**
        @version 0.9.1
        */
        virtual void onException();
    protected:
        ClientIO() {}
    protected:
        ACE_Reactor * mReactor;
        SocketClientPrc * mIOPrc;
        MSTbool mAbort;
        MSTbool mStop;
        CPrcCB mMsgPrc;
        String mIP;
        String mLocalIP;
        Mui16 mPort;
        Mui16 mLocalPort;
        Mi32 mMsgBaseIndex;
    };
}

#endif
