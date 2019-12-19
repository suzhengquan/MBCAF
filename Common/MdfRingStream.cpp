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

#include "MdfRingStream.h"
#include <stdlib.h>
#include <string.h>

namespace Mdf
{
    //-----------------------------------------------------------------------
	RingStream::RingStream(RingBuffer * buf, Mui32 pos)
    {
        mData = buf;
    }
    //-----------------------------------------------------------------------
    void RingStream::write(const std::string & in)
    {
        Mui32 cnt = in.length();
        write(cnt);
		if (cnt)
		{
			writeByte(in.data(), cnt);
		}
    }
    //-----------------------------------------------------------------------
    void RingStream::read(std::string & out) const
    {
		Mui32 cnt;
        read(&cnt);
		if (cnt)
		{
			char * temp = (char *)malloc(cnt);
			readByte(temp, cnt);
			out.assign(temp, cnt);
			free(temp);
		}
		else
		{
			out = "";
		}
    }
    //-----------------------------------------------------------------------
    void RingStream::writeData(Mui8 * in, Mui32 cnt)
    {
        write(cnt);
		if (cnt)
		{
			writeByte(in, cnt);
		}
    }
    //-----------------------------------------------------------------------
    void RingStream::readData(Mui32 & len, Mui8 *& out) const
    {
        read(&len);
		if (len)
		{
			out = (Mui8 *)malloc(len);
			readByte(out, len);
		}
		else
		{
			out = 0;
		}
    }
    //-----------------------------------------------------------------------
}