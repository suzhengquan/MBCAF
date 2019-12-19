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

#ifndef _MDF_RING_STREAM_H_
#define _MDF_RING_STREAM_H_

#include "MdfPreInclude.h"
#include "NetCore.h"
#include "MdfMemStream.h"
#include "MdfRingBuffer.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class MdfNetAPI RingStream
    {
    public:
		RingStream(RingBuffer * buf, Mui32 pos);
        ~RingStream() {}

		/**
		@version 0.9.1
		*/
        inline Mui8 * getData()
        { 
			assert(mData);
			return mData->getBuffer();
        }

		/**
		@version 0.9.1
		*/
        inline Mui32 getCurrent()
        { 
			assert(mData);
            return mData->getWriteSize();
        }

		/**
		@version 0.9.1
		*/
        inline Mui32 getSize()
        { 
			assert(mData);
            return mData->getAllocSize();
        }

        inline void write(Mi8 data) 
        { 
            writeByte(&data, 1); 
        }

        inline void read(Mi8 * data) const
        {
            readByte(data, 1);
        }

        inline void write(Mui8 data)
        { 
            writeByte(&data, 1); 
        }

        inline void read(Mui8 * data) const
        {
            readByte(data, 1);
        }

        inline void write(Mi16 data)
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[2];
				buf[0] = (Mui8)(data >> 8);
				buf[1] = (Mui8)(data & 0xFF);
				writeByte(buf, 2);
			}
			else
			{
				writeByte(&data, 2);
			}
        }

        inline void read(Mi16 * data) const
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[2];
				readByte(buf, 2);
				*data = (buf[0] << 8) | buf[1];
			}
			else
			{
				readByte(data, 2);
			}
        }

        inline void write(Mui16 data)
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[2];
				buf[0] = (Mui8)(data >> 8);
				buf[1] = (Mui8)(data & 0xFF);
				writeByte(buf, 2);
			}
			else
			{
				writeByte(&data, 2);
			}
        }

        inline void read(Mui16 * data) const
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[2];
				readByte(buf, 2);
				*data = (buf[0] << 8) | buf[1];
			}
			else
			{
				readByte(data, 2);
			}
        }

        inline void write(Mi32 data)
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[4];
				buf[0] = (Mui8)(data >> 24);
				buf[1] = (Mui8)((data >> 16) & 0xFF);
				buf[2] = (Mui8)((data >> 8) & 0xFF);
				buf[3] = (Mui8)(data & 0xFF);
				writeByte(buf, 4);
			}
			else
			{
				writeByte(&data, 4);
			}
        }

        inline void read(Mi32 * data) const
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[4];
				readByte(buf, 4);
				*data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
			}
			else
			{
				readByte(data, 4);
			}
        }

        inline void write(Mui32 data)
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[4];
				buf[0] = (Mui8)(data >> 24);
				buf[1] = (Mui8)((data >> 16) & 0xFF);
				buf[2] = (Mui8)((data >> 8) & 0xFF);
				buf[3] = (Mui8)(data & 0xFF);
				writeByte(buf, 4);
			}
			else
			{
				writeByte(&data, 4);
			}
        }

        inline void read(Mui32 * data) const
        {
			if(MemStream::isFlipData())
			{
				Mui8 buf[4];
				readByte(buf, 4);
				*data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
			}
			else
			{
				readByte(data, 4);
			}
        }

		/**
		@version 0.9.1
		*/
        void write(const std::string & in);

		/**
		@version 0.9.1
		*/
        void read(std::string & out) const;

		/**
		@version 0.9.1
		*/
        void writeData(Mui8 * in, Mui32 cnt);

		/**
		@param out using free() until useless
		@version 0.9.1
		*/
        void readData(Mui32 & len, Mui8 *& out) const;

		/**
		@version 0.9.1
		*/
		inline void writeByte(const void * buf, Mui32 cnt)
		{
			assert(mData);
			mData->write(buf, cnt);
		}

		/**
		@version 0.9.1
		*/
		void readByte(void * buf, Mui32 cnt) const
		{
			assert(mData);
			mData->read(buf, cnt);
		}
    private:
        RingBuffer * mData;
    };
}
#endif
