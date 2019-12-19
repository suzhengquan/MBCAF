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

#include "MdfRingBuffer.h"

namespace Mdf
{
    //-----------------------------------------------------------------------
    RingBuffer::RingBuffer()
    {
        mData = 0;
        mAllocSize = 0;
        mWriteSize = 0;
    }
    //-----------------------------------------------------------------------
    RingBuffer::~RingBuffer()
    {
        mAllocSize = 0;
        mWriteSize = 0;
        if(mData)
        {
            free(mData);
            mData = 0;
        }
    }
    //-----------------------------------------------------------------------
    void RingBuffer::enlarge(MCount cnt)
    {
        mAllocSize = mWriteSize + cnt;
        mAllocSize += mAllocSize >> 2;
        if (mData)
        {
            Mui8 * nbuf = (Mui8*)realloc(mData, mAllocSize);
            mData = nbuf;
        }
        else
        {
            mData = (Mui8*)malloc(mAllocSize);
        }

    }
    //-----------------------------------------------------------------------
    void RingBuffer::readSkip(MCount cnt)
    {
        if (cnt > mWriteSize)
            cnt = mWriteSize;

        mWriteSize -= cnt;
        memmove(mData, mData + cnt, mWriteSize);
    }
    //-----------------------------------------------------------------------
    void RingBuffer::writeSkip(MCount cnt)
    { 
        mWriteSize += cnt;
    }
    //-----------------------------------------------------------------------
    Mui32 RingBuffer::write(const void * in, MCount cnt)
    {
        if (mWriteSize + cnt > mAllocSize)
        {
            enlarge(cnt);
        }

        if (in)
        {
            memcpy(mData + mWriteSize, in, cnt);
        }

        mWriteSize += cnt;

        return cnt;
    }
    //-----------------------------------------------------------------------
    Mui32 RingBuffer::read(void * out, MCount cnt) const
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
}