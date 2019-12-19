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

#ifndef _MDF_ServerConnect_H_
#define _MDF_ServerConnect_H_

#include "MdfPreInclude.h"
#include "MdfServerIO.h"

#define KICK_FROM_ROUTE_SERVER         1
#define MAX_ONLINE_FRIEND_CNT        100

namespace Mdf
{
    enum MessageExtDataType
    {
        MEDT_Handle                 = 1,
        MEDT_HandleProto            = 2,
        MEDT_ProtoToPush            = 3,
        MEDT_ProtoToFile            = 4,
    };

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

        /**
        @version 0.9.1
        */
        inline ConnectID getHandle() const { return mHandle; }

        /**
        @version 0.9.1
        */
        inline Mui8 * getBuffer() const { return mBuffer.getBuffer(); }

        /**
        @version 0.9.1
        */
        inline Mui32 getSize() const { return mBuffer.getWriteSize(); }
    private:
        RingBuffer mBuffer;
        ConnectID mHandle;
    };

    class MessageExtData
    {
    public:
        MessageExtData(Mui32 type, ConnectID handle, Mui32 extsize, Mui8 * extdata)
        {
            MemStream os(&mBuffer, 0);
            os.wirte(type);
            os.writeByte(&handle, sizeof(ConnectID));
            os.WriteData(extdata, extsize);
        }

        MessageExtData(Mui8 * data, Mui32 size)
        {
            MemStream is(data, size);
            is.read(&mType);
            is.readByte(&mHandle, sizeof(ConnectID));
            is.readData(mProtoData, mProtoSize);
        }

        /**
        @version 0.9.1
        */
        inline Mui32 getType() const { return mType; }

        /**
        @version 0.9.1
        */
        inline Mui8 * getBuffer() const { return mBuffer.getBuffer(); }

        /**
        @version 0.9.1
        */
        inline Mui32 getSize() const { return mBuffer.getWriteSize(); }

        /**
        @version 0.9.1
        */
        inline ConnectID getHandle() const { return mHandle; }

        /**
        @version 0.9.1
        */
        inline Mui32 getProtoSize() const { return mProtoSize; }

        /**
        @version 0.9.1
        */
        inline Mui8 * getProtoData() const { return mProtoData; }
    private:
        RingBuffer mBuffer;
        Mui32 mType;
        Mui32 mHandle;
        Mui32 mProtoSize;
        Mui8 * mProtoData;
    };

    typedef struct
    {
        Mui32 mMessageID;
        Mui32 mFromID;
        Mui64 mTimeStamp;
    } MessageACK;

    class User;

    /**
    @version 0.9.1
    */
    class ServerConnect : public ServerIO
    {
    public:
        ServerConnect();
        virtual ~ServerConnect();

        /**
        @version 0.9.1
        */
        const String & getLoginID() const { return mLoginID; }

        /**
        @version 0.9.1
        */
        void setID(Mui32 uid) { mUserID = uid; }

        /**
        @version 0.9.1
        */
        Mui32 getUserID() const { return mUserID; }

        /**
        @version 0.9.1
        */
        Mui32 getClientType() const { return mClientType; }

        /**
        @version 0.9.1
        */
        void setOpen(bool set) { mOpen = set; }

        /**
        @version 0.9.1
        */
        bool isOpen() const { return mOpen; }

        /**
        @version 0.9.1
        */
        void setKickOff(bool set) { mKick = set; }

        /**
        @version 0.9.1
        */
        bool isKickOff() const { return mKick; }

        /**
        @version 0.9.1
        */
        void setOnlineState(Mui32 state) { mOnlineState = state; }

        /**
        @version 0.9.1
        */
        Mui32 getOnlineState() const { return mOnlineState; }

        /**
        @version 0.9.1
        */
        virtual void onConnect();

        /**
        @version 0.9.1
        */
        virtual void onClose();

        /**
        @version 0.9.1
        */
        virtual void onTimer(TimeDurMS tick);

        /**
        @version 0.9.1
        */
        virtual void onMessage(Message * msg);

        /**
        @version 0.9.1
        */
        void addSend(Mui32 msgid, Mui32 fromid);

        /**
        @version 0.9.1
        */
        void removeSend(Mui32 msgid, Mui32 fromid);

        /**
        @version 0.9.1
        */
        void SendUserStatusUpdate(Mui32 state);
    private:
        void prcHeartBeat(MdfMessage * msg);
        void prcObjectLoginQ(MdfMessage * msg);
        void prcObjectLogoutQ(MdfMessage * msg);
        void prcBubbyRecentSessionQ(MdfMessage * msg);
        void prcMessageData(MdfMessage * msg);
        void prcMessageDataACK(MdfMessage * msg);
        void prcTimeQ(MdfMessage * msg);
        void prcMessageListQ(MdfMessage * msg);
        void prcMessageQ(MdfMessage * msg);
        void prcUnReadCountQ(MdfMessage * msg);
        void prcReadACK(MdfMessage * msg);
        void prcRecentMessageQ(MdfMessage * msg);
        void prcP2PMessage(MdfMessage * msg);
        void prcBubbyObjectInfoQ(MdfMessage * msg);
        void prcBuddyObjectStateQ(MdfMessage * msg);
        void prcBubbyRemoveSessionQ(MdfMessage * msg);
        void prcBuddyObjectListQ(MdfMessage * msg);
        void prcBuddyAvatarQ(MdfMessage * msg);
        void prcBuddyChangeSignatureQ(MdfMessage * msg);

        void prcTrayMsgQ(MdfMessage * msg);
        void prcKickPCClient(MdfMessage * msg);
        void prcBuddyOrganizationQ(MdfMessage * msg);
        void prcPushShieldSQ(MdfMessage * msg);
        void prcPushShieldQ(MdfMessage * msg);
    private:
        list<MessageACK> mSendList;
        Mui32 mUserID;
        String mLoginID;
        Mui64 mLoginTime;
        String mClientVersion;
        Mui32 mMsgPerSecond;
        Mui32 mClientType;
        Mui32 mOnlineState;
        bool mOpen;
        bool mKick;
    };

    void setupSignal();
}
#endif
