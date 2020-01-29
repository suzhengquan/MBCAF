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

#ifndef _MDF_MESSAGE_H_
#define _MDF_MESSAGE_H_

#include "MdfPreInclude.h"
#include "MdfMemStream.h"
#include "google/protobuf/message_lite.h"

namespace Mdf
{
    #define M_MSG(type, msg) (type << 8 | msg)
    #define SBMSG(msg) (0x100 | SBID_##msg)
    #define MSMSG(msg) (0x200 | MSID_##msg)
    #define HSMSG(msg) (0x300 | HSID_##msg)
	typedef struct
	{
		Mui32   mLength;			// 包大小
		Mui8    mMsgType;			// 协议类型
		Mui8    mMsgID;				// 协议ID
		Mui16   mSeqIdx;            // 包序
	} MdfMsgHeader;

    /** Application layer messages
    @version 0.9.1
    */
    class MdfNetAPI Message
    {
    public:
        Message();
        Message(Mui8 * buf, Mui32 len, bool copydata = true);
        virtual ~Message();

        /**
        @version 0.9.1
        */
        Mui32 write(const void * in, MCount size);

        /**
        @version 0.9.1
        */
        Mui32 read(void * out, MCount cnt) const;

        /*
        @version 0.9.1
        */
        void readSkip(MCount cnt);

        /*
        @version 0.9.1
        */
        void writeSkip(MCount cnt);

        /** 
        @version 0.9.1
        */
        inline Mui8 * getBuffer() const { return mData; }

        /*
        @version 0.9.1
        */
        inline MCount getWriteSize() const { return mWriteSize; }

        /**
        @version 0.9.1
        */
        virtual Mui32 getSize() const { return mWriteSize; }

        /**
        @version 0.9.1
        */
        inline void clear() { mWriteSize = 0; }

        /**
        @version 0.9.1
        */
        virtual void setProto(const google::protobuf::MessageLite * msg);

        /**
        @version 0.9.1
        */
        virtual bool toProto(google::protobuf::MessageLite * out) const;

        /**
        @version 0.9.1
        */
        virtual Message * create(Mui8 * buf, Mui32 len) const;
    protected:
        /**
        @version 0.9.1
        */
        void enlarge(MCount cnt);
    protected:
        Mui8 * mData;
        Mui32 mAllocSize;
        mutable Mui32 mWriteSize;
		bool mCopyData;
    };

    /** Mdf Application layer messages
    @version 0.9.1
    */
    class MdfNetAPI MdfMessage : public Message
    {
    public:
        MdfMessage();
        MdfMessage(Mui8 * buf, Mui32 len, bool copydata = true, bool readhead = true);
        virtual ~MdfMessage();

        /**
        @version 0.9.1
        */
        inline Mui8 * getContent() const
        {
            return mData + sizeof(MdfMsgHeader);
        }

        /**
        @version 0.9.1
        */
        inline MCount getContentSize() const
        {
            return mWriteSize - sizeof(MdfMsgHeader);
        }

        /**
        @version 0.9.1
        */
        inline void setSize(Mui32 size)
        {
            MemStream::write(mData, size);
        }

        /**
        @version 0.9.1
        */
        inline void setMessageType(Mui8 msgtype)
        {
            mMsgType = msgtype;
            mCmdID = (mMsgType << 8) | mMsgID;
            MemStream::write(mData + 4, msgtype);
        }

        /**
        @version 0.9.1
        */
        inline Mui8 getMessageType() const
        {
            return mMsgType;
        }

        /**
        @version 0.9.1
        */
        inline void writeMessageType(Mui8 msgtype)
        {
            MemStream::write(mData + 4, msgtype);
        }

        /**
        @version 0.9.1
        */
        inline void setMessageID(Mui8 msgid)
        {
            mMsgID = mMsgID;
            mCmdID = (mMsgType << 8) | mMsgID;
            MemStream::write(mData + 5, msgid);
        }

        /**
        @version 0.9.1
        */
        inline Mui8 getMessageID() const
        {
            return mMsgID;
        }

        /**
        @version 0.9.1
        */
        inline void writeMessageID(Mui8 msgid)
        {
            MemStream::write(mData + 5, msgid);
        }

        /**
        @version 0.9.1
        */
        inline void setCommandID(Mui16 cmdid)
        {
            mMsgType = cmdid >> 8;
            mMsgID = cmdid & 0xFF;
            MemStream::write(mData + 4, cmdid);
        }

        /**
        @version 0.9.1
        */
        inline Mui16 getCommandID() const
        {
            return mCmdID;
        }

        /**
        @version 0.9.1
        */
        inline void writeCommandID(Mui16 cmdid)
        {
            MemStream::write(mData + 4, cmdid);
        }

        /**
        @version 0.9.1
        */
        inline void setSeqIdx(Mui16 seq)
        {
            MemStream::write(mData + 6, seq);
        }

        /**
        @version 0.9.1
        */
        inline Mui16 getSeqIdx() const
        {
            return mSeqIdx;
        }

        /**
        @version 0.9.1
        */
        virtual void setProto(const google::protobuf::MessageLite * in);

        /**
        @version 0.9.1
        */
        virtual bool toProto(google::protobuf::MessageLite * out) const;

        /**
        @version 0.9.1
        */
        virtual Message * create(Mui8 * buf, Mui32 len) const;
    protected:
        Mui32 mLength;
        Mui8 mMsgType;
        Mui8 mMsgID;
        Mui16 mSeqIdx;
        Mui16 mCmdID;
    };
}

#endif