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
    //-----------------------------------------------------------------------
    // RingBuffer
    //-----------------------------------------------------------------------
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
    //-----------------------------------------------------------------------
    // RingLoopBuffer
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    RingLoopBuffer::RingLoopBuffer()
    {
        mData = NULL;
        mAllocSize = 0;
        mWriteSize = 0;
        mHead = 0;
        mTail = 0;
    }
    //-----------------------------------------------------------------------
    RingLoopBuffer::~RingLoopBuffer()
    {
        destroy_buffer();
    }
    //-----------------------------------------------------------------------
    bool RingLoopBuffer::alloc(Mui32 nsize)
    {
        free();
        int dstsize = (nsize / 4 + 1) * 4;
        mData = (Mui8 *)malloc(dstsize);
        memset(mData, 0, dstsize);
        mAllocSize = dstsize;
        mWriteSize = 0;
        return true;
    }
    //-----------------------------------------------------------------------
    void RingLoopBuffer::free()
    {
        if (mData != NULL)
        {
            free(mData);
            mData = NULL;
            mHead = 0;
            mTail = 0;
            mWriteSize = 0;
        }
    }
    //-----------------------------------------------------------------------
    Mui32 RingLoopBuffer::write(const void * in, int size)
    {
        if (mAllocSize - mWriteSize < size)
        {
            return 0;
        }

        if (mTail < mHead)
        {
            memcpy(&mData[mTail], in, size);
        }
        else
        {
            int nrest_tail = mAllocSize - mTail;
            if (nrest_tail >= size)
            {
                memcpy(&mData[mTail], in, size);
            }
            else
            {
                memcpy(&mData[mTail], in, nrest_tail);
                memcpy(&mData[0], &in[nrest_tail], size - nrest_tail);
            }
        }
        mTail = (mTail + size) % mAllocSize;
        mWriteSize += size;
        return size;
    }
    //-----------------------------------------------------------------------
    Mui32 RingLoopBuffer::read(void * out, int nbuffer_size) const
    {
        int dstsize = (nbuffer_size < getWriteSize() ? nbuffer_size : getWriteSize());
        if (mHead <= mTail)
        {
            memcpy(out, &mData[mHead], dstsize);
        }
        else
        {
            int nrestsize = mAllocSize - mHead;
            if (dstsize <= nrestsize)
            {
                memcpy(out, &mData[mHead], dstsize);
            }
            else
            {
                memcpy(out, &mData[mHead], nrestsize);
                memcpy(&out[nrestsize], &mData[0], dstsize - nrestsize);
            }
        }
        mWriteSize -= dstsize;
        mHead = (mHead + dstsize) % mAllocSize;
        return dstsize;
    }
    //-----------------------------------------------------------------------
    Mui32 RingLoopBuffer::peek(void * out, int size) const
    {
        int dstsize = (size < mWriteSize ? size : mWriteSize);
        if (mHead <= mTail)
        {
            memcpy(out, &mData[mHead], dstsize);
        }
        else
        {
            int nrestsize = mAllocSize - mHead;
            if (dstsize <= nrestsize)
            {
                memcpy(out, &mData[mHead], dstsize);
            }
            else
            {
                memcpy(out, &mData[mHead], nrestsize);
                memcpy(&out[nrestsize], &mData[0], dstsize - nrestsize);
            }
        }
        return dstsize;
    }
    //-----------------------------------------------------------------------
}