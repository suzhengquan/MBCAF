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

#ifndef _MDF_APNService_H_
#define _MDF_APNService_H_

#include "MdfPreInclude.h"
#include "MdfRingBuffer.h"
#include "MdfPushMessage.h"

/* APNS Gateway Response */
/*
0   No errors encountered
1   Processing error
2   Missing device token
3   Missing topic
4   Missing payload
5   Invalid token size
6   Invalid topic size
7   Invalid payload size
8   Invalid token
10  Shutdown
255 None (unknown)
*/
#ifdef MDF_LINUX
#define PACKED __attribute__((packed))
#define PACKBEGIN
#define PACKEND
#else
#define PACKED
#define PACKBEGIN #pragma pack(push,1)
#define PACKEND #pragma pack(pop) 
#endif
namespace Mdf
{
    /**
    @version 0.9.1
    */
    class MdfNetAPI PushMessage : public Message
    {
    public:
        PushMessage()
        {
            mHeadSize = 0;
            mContentSize = 0;
        }
        virtual ~PushMessage() {}

        /**
        @version 0.9.1
        */
        Mui32 getSize() const { return mHeadSize + mContentSize; }

        /**
        @version 0.9.1
        */
        bool checkVaild() const 
        {
            if (mData.getWriteSize() >= getSize())
            {
                return true;
            }
            return false;
        }
    protected:
        Mui32 mHeadSize;
        Mui32 mContentSize;
    };

    PACKBEGIN
    typedef struct AGateWay_t
    {
        Mui8 command_id;
        Mui32 frame_length;
    }PACKED AGateWay;
    PACKEND

    class AppleGateWayQ : public PushMessage
    {
    public:
        AppleGateWayQ();
        virtual ~AppleGateWayQ();

        bool proto();

        /**
        @version 0.9.1
        */
        inline void setDeviceToken(const String & data) { mDeviceToken = data; }

        /**
        @version 0.9.1
        */
        inline const String & getDeviceToken() const { return mDeviceToken; }

        /**
        @version 0.9.1
        */
        inline void setNotificationID(Mui32 id) { mNotificationID = id; }

        /**
        @version 0.9.1
        */
        inline Mui32 getNotificationID() const { return mNotificationID; }

        /**
        @version 0.9.1
        */
        inline void setExpirationDate(Mui32 data) { mExpiredData = data; }

        /**
        @version 0.9.1
        */
        inline Mui32 getExpirationDate() const { return mExpiredData; }

        /**
        @version 0.9.1
        */
        inline void setPriority(char level) { mPriority = level; }

        /**
        @version 0.9.1
        */
        inline char getPriority() const { return mPriority; }

        /**
        @version 0.9.1
        */
        inline void setSound(bool set) { mSound = set; }

        /**
        @version 0.9.1
        */
        inline bool getSound() const { return mSound; }

        /**
        @version 0.9.1
        */
        inline void setCustomData(const String & data) { mCustomData = data; }

        /**
        @version 0.9.1
        */
        inline const String & getCustomData() const { return mCustomData; }

        /**
        @version 0.9.1
        */
        inline void setBadge(Mui32 badge) { mBadge = badge; }

        /**
        @version 0.9.1
        */
        inline Mui32 getBadge() const { return mBadge; }

        /**
        @version 0.9.1
        */
        inline void setAlterBody(const String & data) { mAlterBody = data; }

        /**
        @version 0.9.1
        */
        inline const String & getAlterBody() const { return mAlterBody; }

        /**
        @version 0.9.1
        */
        inline void setActionLocKey(const String & data) { mActionLocKey = data; }

        /**
        @version 0.9.1
        */
        inline const String & getActionLocKey() const { return mActionLocKey; }

        /**
        @version 0.9.1
        */
        inline void setLocKey(const String & data) { mLocKey = data; }

        /**
        @version 0.9.1
        */
        inline const String & getLocKey() const { return mLocKey; }

        /**
        @version 0.9.1
        */
        inline void setLocArgsList(const list<String> & list) { m_LocArgsList = list; }

        /**
        @version 0.9.1
        */
        inline const list<String> & getLocArgsList() const { return m_LocArgsList; }

        /**
        @version 0.9.1
        */
        inline void setLaunchImage(const String & data) { mLaunchImage = data; }

        /**
        @version 0.9.1
        */
        inline const String & getLaunchImage() const { return mLaunchImage; }
    private:
        String buildJson();
    private:
        AGateWay mAGateWayHead;
        String   mDeviceToken;
        Mui32    mBadge;
        Mui32    mNotificationID;
        Mui32    mExpiredData;
        String   mCustomData;
        String   mAlterBody;
        String   mActionLocKey;
        String   mLocKey;
        list<String> m_LocArgsList;
        String   mLaunchImage;
        char     mPriority;
        bool     mSound;
    };

    class AppleGateWayA : public PushMessage
    {
    public:
        AppleGateWayA();
        ~AppleGateWayA();

        /**
        @version 0.9.1
        */
        inline Mui8 getMessageID() const { return mMsgID; }

        /**
        @version 0.9.1
        */
        inline Mui8 getStatus() const { return mState; }

        /**
        @version 0.9.1
        */
        inline Mui32 getNotificationID() const { return mNotificationID; }

        /// @copydetails PushMessage::create
        Message * create(Mui8 * buf, Mui32 len) const;
    private:
        Mui8  mMsgID;
        Mui8  mState;
        Mui32 mNotificationID;
    };

    class AppleFeedBackA : public PushMessage
    {
    public:
        AppleFeedBackA();
        virtual ~AppleFeedBackA();

        /**
        @version 0.9.1
        */
        inline Mui32 getTime() const { return mTime; }

        /**
        @version 0.9.1
        */
        inline Mui16 getTokenSize() const { return mTokenSize; }

        /**
        @version 0.9.1
        */
        inline const String & getToken() const { return mToken; }

        /// @copydetails PushMessage::create
        Message * create(Mui8 * buf, Mui32 len) const;
    private:
        String mToken;
        Mui32 mTime;
        Mui16 mTokenSize;
    };

    class APNServerConnect
    {
    public:
        APNServerConnect();
        virtual ~APNServerConnect();

        /**
        @version 0.9.1
        */
        inline void setCertFile(const String & certfile) { mCertFile = certfile; }

        /**
        @version 0.9.1
        */
        inline const String & getCertFile() const { return mCertFile; }

        /**
        @version 0.9.1
        */
        inline void setKeyPath(const String & keyfile) { mKeyFile = keyfile; }

        /**
        @version 0.9.1
        */
        inline const String & getKeyPath() const { return mKeyFile; }

        /**
        @version 0.9.1
        */
        inline void setKeyPassword(const String & keypw) { mKeyPW = keypw; }

        /**
        @version 0.9.1
        */
        inline const String & getKeyPassword() const { return mKeyPW; }

        /**
        @version 0.9.1
        */
        inline void setSandBox(bool set) { mSandBox = set; }

        /**
        @version 0.9.1
        */
        inline bool getSandBox() const { return mSandBox; }

        /**
        @version 0.9.1
        */
        inline const String & getGatewayIP() const { return mGatewayIP; }

        /**
        @version 0.9.1
        */
        inline Mui32 getGatewayPort() const { return mGatewayPort; }

        /**
        @version 0.9.1
        */
        inline const String & getFeedbackIP() const { return mFeedbackIP; }

        /**
        @version 0.9.1
        */
        inline Mui32 getFeedbackPort() const { return mFeedbackPort; }

        /**
        @version 0.9.1
        */
        bool start(ACE_Reactor * react);

        /**
        @version 0.9.1
        */
        bool stop();

        /**
        @version 0.9.1
        */
        bool sendGateway(const Mui8 * in, MCount size);

        /**
        @version 0.9.1
        */
        bool reconnectGateway();

        /**
        @version 0.9.1
        */
        bool reconnectFeedback();

        /**
        @version 0.9.1
        */
        void checkGateway();

        /**
        @version 0.9.1
        */
        void checkFeedback();
    private:
        bool resolvePNIP();
    private:
        String mCertFile;
        String mKeyFile;
        String mKeyPW;
        String mGatewayIP;
        String mFeedbackIP;
        Mui32 mGatewayPort;
        Mui32 mFeedbackPort;
        ClientIO * mGatewayClient;
        SSLClientPrc * mGatewayPrc;
        ClientIO * mFeedbackClient;
        SSLClientPrc * mFeedbackPrc;
        ConnectManager::TimerListener * mRGatewayTimer;
        ConnectManager::TimerListener * mRFeedbackTimer;
        ConnectManager::TimerListener * mCGatewayTimer;
        ConnectManager::TimerListener * mCFeedbackTimer;
        bool mSandBox;
    };
}
#endif
