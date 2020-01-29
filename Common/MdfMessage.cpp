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

#include "MdfMessage.h"
#include "MdfLogManager.h"

namespace Mdf
{
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Message
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    Message::Message()
    {
        mData = NULL;
        mAllocSize = 0;
        mWriteSize = 0;
        mCopyData = true;
    }
    //-----------------------------------------------------------------------
    Message::Message(Mui8 * buf, Mui32 len, bool copydata)
    {
        mWriteSize = len;

        if (buf && copydata)
        {
            mAllocSize = 128;
            if (mAllocSize < len)
                mAllocSize = len;
            mData = (Mui8 *)malloc(mAllocSize);
            memcpy(mData, buf, len);
            mCopyData = true;
        }
        else
        {
            mAllocSize = 0;
            mData = buf;
            mCopyData = false;
        }
    }
    //-----------------------------------------------------------------------
    Message::~Message()
    {
        mAllocSize = 0;
        mWriteSize = 0;
        if (mData && mCopyData)
        {
            free(mData);
            mData = NULL;
        }
    }
    //-----------------------------------------------------------------------
    Mui32 Message::write(const void * in, MCount size)
    {
        if (mWriteSize + size > mAllocSize)
        {
            enlarge(size);
        }

        if (in)
        {
            memcpy(mData + mWriteSize, in, size);
        }

        mWriteSize += size;

        return size;
    }
    //-----------------------------------------------------------------------
    Mui32 Message::read(void * out, MCount cnt) const
    {
        if (cnt > mWriteSize)
            cnt = mWriteSize;

        if (out)
            memcpy(out, mData, cnt);

        mWriteSize -= cnt;
        memmove(mData, mData + cnt, mWriteSize);
        return cnt;
    }
    //-----------------------------------------------------------------------
    void Message::readSkip(MCount cnt)
    {
        if (cnt > mWriteSize)
            cnt = mWriteSize;

        mWriteSize -= cnt;
        memmove(mData, mData + cnt, mWriteSize);
    }
    //-----------------------------------------------------------------------
    void Message::writeSkip(MCount cnt)
    {
        mWriteSize += cnt;
    }
    //-----------------------------------------------------------------------
    Message * Message::create(Mui8 * buf, Mui32 len) const
    {
        Message * msg = new Message(buf, len, true);
        return msg;
    }
    //-----------------------------------------------------------------------
    void Message::enlarge(MCount cnt)
    {
        mAllocSize = mWriteSize + cnt;
        mAllocSize += mAllocSize >> 2;
        if (mCopyData)
        {
            Mui8 * nbuf = (Mui8*)realloc(mData, mAllocSize);
            mData = nbuf;
        }
        else
        {
            if (mWriteSize > 0)
            {
                Mui8 * nbuf = (Mui8*)malloc(mAllocSize);
                memcpy(nbuf, mData, mWriteSize);
                mData = nbuf;
            }
            else
            {
                mData = (Mui8*)malloc(mAllocSize);
            }
            mCopyData = true;
        }

    }
    //-----------------------------------------------------------------------
    void Message::setProto(const google::protobuf::MessageLite * in)
    {
        Mui32 msgsize = in->ByteSize();

        if (mData && mCopyData)
        {
            if (msgsize > mAllocSize)
            {
                mAllocSize = msgsize;

                Mui8 * new_buf = (Mui8 *)realloc(mData, mAllocSize);
                mData = new_buf;
            }
        }
        else
        {
            mCopyData = true;
            mAllocSize = msgsize;
            mData = (Mui8 *)malloc(mAllocSize);
        }

        if (!in->SerializeToArray(mData, msgsize))
        {
            Mlog("pb msg miss required fields.");
        }
    }
    //-----------------------------------------------------------------------
    bool Message::toProto(google::protobuf::MessageLite * out) const
    {
        return out->ParseFromArray(mData, mWriteSize);
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // MdfMessage
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    MdfMessage::MdfMessage()
    {
        mLength = 0;
        mMsgType = 0;
        mMsgID = 0;
        mSeqIdx = 0;
        mCmdID = 0;
    }
    //-----------------------------------------------------------------------
    MdfMessage::MdfMessage(Mui8 * buf, Mui32 len, bool copydata, bool readhead) :
        Message(buf, len, copydata)
    {
        if (buf && readhead)
        {
            MemStream is(buf, sizeof(MdfMsgHeader));

            is.read(&mLength);
            is.read(&mMsgType);
            is.read(&mMsgID);
            is.read(&mSeqIdx);
            mCmdID = (mMsgType << 8) | mMsgID;
        }
    }
    //-----------------------------------------------------------------------
    MdfMessage::~MdfMessage()
    {
    }
    //-----------------------------------------------------------------------
    Message * MdfMessage::create(Mui8 * buf, Mui32 len) const
    {
        if(len < sizeof(MdfMsgHeader))
            return 0;

        Mui32 msglen;
        MemStream::read(buf, msglen);
        if (msglen > len)
        {
            return 0;
        }

        MdfMessage * msg = new MdfMessage(buf, msglen, true, true);

        return msg;
    }
    //-----------------------------------------------------------------------
    void MdfMessage::setProto(const google::protobuf::MessageLite * msg)
    {
        Mui32 msgsize = msg->ByteSize();
        mWriteSize = msgsize + sizeof(MdfMsgHeader);

        if (mData && mCopyData)
        {
            if (mWriteSize > mAllocSize)
            {
                mAllocSize = mWriteSize;

                Mui8 * new_buf = (Mui8 *)realloc(mData, mAllocSize);
                mData = new_buf;
            }
        }
        else
        {
			mCopyData = true;
            mAllocSize = mWriteSize;
            mData = (Mui8 *)malloc(mAllocSize);
        }

        if (!msg->SerializeToArray(mData + sizeof(MdfMsgHeader), msgsize))
        {
            Mlog("pb msg miss required fields.");
        }

        MemStream::write(mData, mWriteSize);
        MemStream::write(mData + 4, mMsgType);
        MemStream::write(mData + 5, mMsgID);
        MemStream::write(mData + 6, mSeqIdx);
    }
    //-----------------------------------------------------------------------
    bool MdfMessage::toProto(google::protobuf::MessageLite * out) const
    {
        return out->ParseFromArray(mData + sizeof(MdfMsgHeader), mWriteSize - sizeof(MdfMsgHeader));
    }
    //-----------------------------------------------------------------------
}