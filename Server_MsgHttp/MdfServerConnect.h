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

#ifndef _MDF_SERVERCONNECT_H__
#define _MDF_SERVERCONNECT_H__

#include "MdfPreInclude.h"
#include "MdfClientIO.h"
#include "MdfStrUtil.h"

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class HandleExtData
    {
    public:
        HandleExtData(ConnectID handle)
        {
            MemStream os(&mBuffer, 0);
            os.writeByte(&handle, sizeof(ConnectID));
        }

        HandleExtData(Mui8 * data, Mui32 size)
        {
            MemStream is(data, size);
            is.readByte(&mHandle, sizeof(ConnectID));
        }

        inline ConnectID getHandle() { return mHandle; }

        inline Mui8 * getBuffer() { return mBuffer.getBuffer(); }

        inline Mui32 getSize() { return mBuffer.getWriteSize(); }
    private:
        RingBuffer mBuffer;
        ConnectID mHandle;
    };

    /**
    @version 0.9.1
    */
    class ServerConnect : public ClientIO
    {
    public:
        ServerConnect();
        virtual ~ServerConnect();

        /// @copydetails ClientIO::onConnect
        void onConnect();

        /// @copydetails ClientIO::onClose
        void onClose();

        /// @copydetails ClientIO::onTimer
        void onTimer(TimeDurMS tick);
    };
}
#endif
