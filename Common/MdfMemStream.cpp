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

#include "MdfMemStream.h"
#include <stdlib.h>
#include <string.h>

namespace Mdf
{
	//-----------------------------------------------------------------------
	MemStream::MemStream() :
		mAutoDestroy(true),
		mAutoEnlarge(true),
		mStack(false)
	{
		mData = 0;
		mSize = 0;
		mCurrent = 0;
	}
    //-----------------------------------------------------------------------
    MemStream::MemStream(Mui8 * buf, Mui32 len, bool autoDestroy, bool autoEnlarge):
		mStack(false)
    {
		if (buf)
		{
			mAutoDestroy = autoDestroy;
			mAutoEnlarge = autoEnlarge;
			mData = buf;
			mSize = len;
		}
		else
		{
			mAutoDestroy = true;
			mAutoEnlarge = true;
			mData = (Mui8 *)malloc(128);
			buf = mData;
			mSize = 128;
		}
        mCurrent = 0;
    }
	//-----------------------------------------------------------------------
	MemStream::~MemStream() 
	{
		if (mAutoDestroy)
		{
			free(mData);
			mData = 0;
		}
	}
    //-----------------------------------------------------------------------
    void MemStream::read(const Mui8 * buf, Mi16 & out)
    {
		if (isFlipData())
			out = (buf[0] << 8) | buf[1];
		else
			out = *(Mi16 *)buf;
    }
    //-----------------------------------------------------------------------
    void MemStream::read(const Mui8 * buf, Mui16 & out)
    {
		if (isFlipData())
			out = (buf[0] << 8) | buf[1];
		else
			out = *(Mi16 *)buf;
    }
    //-----------------------------------------------------------------------
    void MemStream::read(const Mui8 * buf, Mi32 & out)
    {
		if (isFlipData())
			out = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
		else
			out = *(Mi32 *)buf;
    }
    //-----------------------------------------------------------------------
    void MemStream::read(const Mui8 * buf, Mui32 & out)
    {
		if (isFlipData())
			out = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
		else
			out = *(Mui32 *)buf;
    }
    //-----------------------------------------------------------------------
    void MemStream::write(Mui8 * buf, Mi16 data)
    {
		if (isFlipData())
		{
			buf[0] = (Mui8)(data >> 8);
			buf[1] = (Mui8)(data & 0xFF);
		}
		else
		{
			*(Mi16 *)buf = data;
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::write(Mui8 * buf, Mui16 data)
    {
		if (isFlipData())
		{
			buf[0] = (Mui8)(data >> 8);
			buf[1] = (Mui8)(data & 0xFF);
		}
		else
		{
			*(Mui16 *)buf = data;
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::write(Mui8 * buf, Mi32 data)
    {
		if (isFlipData())
		{
			buf[0] = (Mui8)(data >> 24);
			buf[1] = (Mui8)((data >> 16) & 0xFF);
			buf[2] = (Mui8)((data >> 8) & 0xFF);
			buf[3] = (Mui8)(data & 0xFF);
		}
		else
		{
			*(Mi32 *)buf = data;
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::write(Mui8 * buf, Mui32 data)
    {
		if (isFlipData())
		{
			buf[0] = (Mui8)(data >> 24);
			buf[1] = (Mui8)((data >> 16) & 0xFF);
			buf[2] = (Mui8)((data >> 8) & 0xFF);
			buf[3] = (Mui8)(data & 0xFF);
		}
		else
		{
			*(Mui32 *)buf = data;
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::write(const std::string & in)
    {
        Mui32 size = in.length();

        write(size);
		if(size)
		{
			writeByte(in.data(), size);
		}
    }
	//-----------------------------------------------------------------------
	void MemStream::read(std::string & out) const
	{
		Mui32 temp;
		read(&temp);
		if (temp)
		{
			char * data = (char *)malloc(temp);
			readByte(data, temp);
			out.assign(data, temp);
			free(data);
		}
		else
		{
			out = "";
		}
	}
    //-----------------------------------------------------------------------
    void MemStream::writeData(const Mui8 * in, Mui32 len)
    {
        write(len);
		if (len)
		{
			writeByte(in, len);
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::readData(Mui32 & len, Mui8 *& out) const
    {
        read(&len);
		if (len)
		{
			assert(mCurrent + len < mSize);
			out = (Mui8 *)malloc(len);
			memcpy(out, mData + mCurrent, len);
			mCurrent += len;
		}
		else
		{
			out = 0;
		}
    }
    //-----------------------------------------------------------------------
    void MemStream::readByte(void * out, Mui32 len) const
    {
		assert(mData);
        if(mCurrent + len > mSize)
            return;

		memcpy(out, mData + mCurrent, len);
        mCurrent += len;
    }
    //-----------------------------------------------------------------------
    void MemStream::writeByte(const void * in, Mui32 len)
    {
		assert(mData);
		if (mCurrent + len > mSize)
		{
			if(!enlarge(len))
				return;
		}

		memcpy(mData + mCurrent, in, len);
        mCurrent += len;
    }
	//-----------------------------------------------------------------------
	bool MemStream::enlarge(Mui32 cnt)
	{
		if (mAutoDestroy && mAutoEnlarge)
		{
			mSize = mCurrent + cnt;
			mSize += mSize >> 2;
			mData = (Mui8 *)realloc(mData, mSize);
			return true;
		}
		return false;
	}
	//-----------------------------------------------------------------------
	bool MemStream::isFlipData()
	{
#ifdef M_NET_Endian_N
		return isNetEndian() == false;
#else
		return false;
#endif
	}
	//-----------------------------------------------------------------------
	bool MemStream::isNetEndian()
	{
		static const bool isNetworkOrder = (htonl(12345) == 12345);
		return isNetworkOrder;
	}
    //-----------------------------------------------------------------------
}