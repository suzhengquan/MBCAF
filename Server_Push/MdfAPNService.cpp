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

#include "MdfAPNService.h"
#include "MdfConnectTypeList.h"
#include "MdfSSLClient.h"
#include "json/json.h"

#define APNS_DEVICE_TOKEN_HEX_LENGTH    64
#define APNS_DEVICE_TOKEN_BINARY_LENGTH 32
#define APNS_PAY_LOAD_MAX_LENGTH        256

#define APNS_FEEDBACK_MSG_TIME_LENGTH   4
#define APNS_FEEDBACK_MSG_TOKEN_LENGTH  2
#define APNS_FEEDBACK_MSG_TOKEN         32

#define APNS_GATEWAY_RESP_LENGTH        6

 /* The device token in binary form, as was registered by the device. */
#define APNS_ITEM_DEVICE_TOKEN          1
/* The JSON-formatted payload.
   The payload must not be null-terminated. */
#define APNS_ITEM_PAY_LOAD              2
/* An arbitrary, opaque value that identifies this notification. This identifier is used for
   reporting errors to your server. */
#define APNS_ITEM_NOTIFICATION_ID       3
/* A UNIX epoch date expressed in seconds (UTC) that identifies when the notification is no longer
   valid and can be discarded.
   If this value is non-zero, APNs stores the notification tries to deliver the notification at
   least once. Specify zero to indicate that the notification expires immediately and that APNs
   should not store the notification at all. */
#define APNS_ITEM_EXPIRATION_DATE       4
/* The notification’s priority. */
#define APNS_ITEM_PRIORITY              5


/* 10 The push message is sent immediately.
   The remote notification must trigger an alert, sound, or badge on the device. It is an error to
   use this priority for a push that contains only the content-available key. */
#define APNS_PRIORITY_IMMEDIATELY       10
   /* 5 The push message is sent at a time that conserves power on the device receiving it. */
#define APNS_PRIORITY_CONSERVE          5

namespace Mdf
{
    typedef struct apn_server_t
    {
        const char * host;
        int port;
    } apn_server;

    static apn_server googlepns[2] =
    {
        {"google.com", 0},
        {"google.com", 0}
    };

    static apn_server applepns[4] =
    {
        {"gateway.sandbox.push.apple.com", 2195},
        {"feedback.sandbox.push.apple.com", 2196},
        {"gateway.push.apple.com", 2195},
        {"feedback.push.apple.com", 2196}
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // AppleGateWayQ
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    AppleGateWayQ::AppleGateWayQ()
    {
        memset(&mAGateWayHead, 0, sizeof(AGateWay));
        mHeadSize = sizeof(AGateWay);
        mAGateWayHead.command_id = (Mui8)2;
        mExpiredData = 1;
        mPriority = APNS_PRIORITY_IMMEDIATELY;
        mSound = true;
        mBadge = 0;
        mNotificationID = 0;
    }
    //-----------------------------------------------------------------------
    AppleGateWayQ::~AppleGateWayQ()
    {
    }
    //-----------------------------------------------------------------------
    bool AppleGateWayQ::proto()
    {
        bool re = false;
        if (getWriteSize() != 0)
        {
            Mlog("push msg serialize failed, databuffer offset: %d.", getWriteSize());
            return re;
        }
        if (mDeviceToken.length() != APNS_DEVICE_TOKEN_HEX_LENGTH)
        {
            Mlog("push msg serialize failed, device token length: %d, token: %s.", mDeviceToken.length(), mDeviceToken.c_str());
            return re;
        }

        String strPayload = buildJson();
        if (strPayload.length() > APNS_PAY_LOAD_MAX_LENGTH || strPayload.length() == 0)
        {
            Mlog("push msg serialize failed, payload length: %d.", strPayload.length());
            return re;
        }

        write(NULL, mHeadSize);

        //device token
        int8_t nItemID = APNS_ITEM_DEVICE_TOKEN;
        int16_t nItemDataLength = htons(APNS_DEVICE_TOKEN_BINARY_LENGTH);
        write((Mui8 *)&nItemID, sizeof(nItemID));
        write((Mui8 *)&nItemDataLength, sizeof(nItemDataLength));
        char szDeviceToken[APNS_DEVICE_TOKEN_HEX_LENGTH + 1] = { 0 };
        strcpy(szDeviceToken, mDeviceToken.c_str());
        int8_t device_token[APNS_DEVICE_TOKEN_BINARY_LENGTH] = { 0 };
        for (Mui32 i = 0, j = 0; i < APNS_DEVICE_TOKEN_BINARY_LENGTH; i++, j += 2)
        {
            int8_t binary = 0;
            char tmp[3] = { szDeviceToken[j], szDeviceToken[j + 1], '\0' };
            sscanf(tmp, "%x", &binary);
            device_token[i] = binary;
        }
        write((Mui8 *)&device_token, sizeof(device_token));

        //pay load
        nItemID = APNS_ITEM_PAY_LOAD;
        nItemDataLength = htons(strPayload.length());
        write((Mui8 *)&nItemID, sizeof(nItemID));
        write((Mui8 *)&nItemDataLength, sizeof(nItemDataLength));
        write(strPayload.c_str(), (int32_t)strPayload.length());

        //notification ID
        nItemID = APNS_ITEM_NOTIFICATION_ID;
        nItemDataLength = htons(sizeof(mNotificationID));
        int32_t nNotificationID = htonl(mNotificationID);
        write((Mui8 *)&nItemID, sizeof(nItemID));
        write((Mui8 *)&nItemDataLength, sizeof(nItemDataLength));
        write((Mui8 *)&nNotificationID, sizeof(nNotificationID));

        //expiration date
        nItemID = APNS_ITEM_EXPIRATION_DATE;
        nItemDataLength = htons(sizeof(mExpiredData));
        int32_t nExpirationDate = htonl(mExpiredData);
        write((Mui8 *)&nItemID, sizeof(nItemID));
        write((Mui8 *)&nItemDataLength, sizeof(nItemDataLength));
        write((Mui8 *)&nExpirationDate, sizeof(nExpirationDate));

        //priority
        nItemID = APNS_ITEM_PRIORITY;
        nItemDataLength = htons(sizeof(mPriority));
        write((Mui8 *)&nItemID, sizeof(nItemID));
        write((Mui8 *)&nItemDataLength, sizeof(nItemDataLength));
        write((Mui8 *)&mPriority, sizeof(mPriority));

        mContentSize = getWriteSize() - mHeadSize;

        mAGateWayHead.frame_length = htonl(mContentSize);
        char * buf = getBuffer();
        memcpy(buf, &mAGateWayHead, sizeof(AGateWay));

        re = true;
        MlogDebug("push msg buffer length: %d, payload length: %d.", getWriteSize(), strPayload.length());
        return re;
    }
    //-----------------------------------------------------------------------
    String AppleGateWayQ::buildJson()
    {
        Json::FastWriter jwriter;
        Json::Value payload_obj, aps_obj, alert_obj;

        if (getAlterBody().length() != 0)
        {
            alert_obj["body"] = Json::Value(getAlterBody());
        }
        if (getActionLocKey().length() != 0)
        {
            alert_obj["action-loc-key"] = Json::Value(getActionLocKey());
        }

        if (getLocKey().length() != 0)
        {
            alert_obj["loc-key"] = Json::Value(getLocKey());
        }

        if (getLocArgsList().size() != 0)
        {
            Json::Value loc_args_array;
            const list<String>& loc_args_list = getLocArgsList();
            for (list<String>::const_iterator it = loc_args_list.begin(); it != loc_args_list.end(); it++)
            {
                loc_args_array.append(*it);
            }
            alert_obj["loc-args"] = Json::Value(loc_args_array);
        }

        if (getLaunchImage().length() != 0)
        {
            alert_obj["launch-image"] = Json::Value(getLaunchImage());
        }

        if (getSound() == false)
        {
            aps_obj["badge"] = Json::Value(getBadge());
        }
        else
        {
            aps_obj["alert"] = alert_obj;
            aps_obj["badge"] = Json::Value(getBadge());
            aps_obj["sound"] = Json::Value("bingbong.aiff");
        }

        payload_obj["aps"] = aps_obj;
        payload_obj["custom"] = Json::Value(getCustomData());
        String re = jwriter.write(payload_obj);
        MlogDebug("%s", re.c_str());
        return re;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // AppleGateWayA
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    AppleGateWayA::AppleGateWayA()
    {
        mHeadSize = 1;
        mContentSize = 5;
        mMsgID = 0;
        mState = 0;
        mNotificationID = 0;
        mCopyData = false;
    }
    //-----------------------------------------------------------------------
    AppleGateWayA::~AppleGateWayA()
    {
    }
    //-----------------------------------------------------------------------
    Message * AppleGateWayA::create(Mui8 * buf, Mui32 len) const
    {
        write(buf, len);
        if(checkVaild())
        {
            memcpy(&mMsgID, buf, sizeof(mMsgID));
            memcpy(&mState, buf + sizeof(mMsgID), sizeof(mState));
            memcpy(&mNotificationID, buf + sizeof(mMsgID) + sizeof(mState), sizeof(mNotificationID));
            mNotificationID = ntohl(mNotificationID);
            return this;
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // AppleFeedBackA
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    AppleFeedBackA::AppleFeedBackA()
    {
        mHeadSize = APNS_FEEDBACK_MSG_TIME_LENGTH + APNS_FEEDBACK_MSG_TOKEN_LENGTH;
        mContentSize = APNS_FEEDBACK_MSG_TOKEN;
        mTime = 0;
        mTokenSize = 0;
        mCopyData = false;
    }
    //-----------------------------------------------------------------------
    AppleFeedBackA::~AppleFeedBackA()
    {
    }
    //-----------------------------------------------------------------------
    Message * AppleFeedBackA::create(Mui8 * buf, Mui32 len) const
    {
        write(buf, len);
        if(checkVaild())
        {
            memcpy(&mTime, buf, sizeof(mTime));
            mTime = ntohl(mTime);
            memcpy(&mTokenSize, buf + sizeof(mTime), sizeof(mTokenSize));
            mTokenSize = ntohs(mTokenSize);
            Mui8 binary_token[32] = { 0 };
            char device_token[APNS_DEVICE_TOKEN_HEX_LENGTH + 1] = { 0 };
            char * p = device_token;
            memcpy(binary_token, buf + sizeof(mTime) + sizeof(mTokenSize), mTokenSize);
            for (Mui32 i = 0; i < APNS_DEVICE_TOKEN_BINARY_LENGTH; i++)
            {
                snprintf(p, 3, "%2.2hhX", binary_token[i]);
                p += 2;
            }
            mToken = device_token;
            return this;
        }
        return 0;
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // AppleGatewayClient
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class AppleGatewayClient : public ClientIO
    {
    public:
        Message * create(Mui8 * buf, Mui32 len)
        {
            AppleGateWayA temp;
            Message * msg = temp.create(buf, len);
            return msg;
        }

        virtual void onClose()
        {
            mRGatewayTimer->setEnable(true);
            mRGatewayTimer->setInterval(5000);
            mRFeedbackTimer->setEnable(true);
            mRFeedbackTimer->setInterval(3000);
        }

        virtual void onMessage(Message * msg)
        {
            AppleGateWayA * temp = static_cast<AppleGateWayA *>(msg);
            Mlog("gateway client resp, cmd id: %u, state: %u, notification id: %u", temp->getCommandID(), temp->getStatus(), temp->getNotificationID());  
        }
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // AppleFeedBackClient
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class AppleFeedBackClient : public ClientIO
    {
    public:
        Message * create(Mui8 * buf, Mui32 len)
        {
            AppleFeedBackA temp;
            Message * msg = temp.create(buf, len);
            return msg;
        }

        virtual void onMessage(Message * msg)
        {
            AppleFeedBackA * temp = static_cast<AppleFeedBackA *>(msg);
            Nui32 time = temp->getTime();
            const String & token = temp->getToken();
            Mlog("apns feedback client recv resp, token: %s.", token.c_str());
        }
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // PNCheckGateway
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class PNCheckGateway : public ConnectManager::TimerListener
    {
    public:
        PNCheckGateway(APNServerConnect * con) :
            mConnect(con) 
        {
        }
    protected:
        void onTimer(TimeDurMS tick)
        {
            mConnect->checkGateway();
        }
    private:
        APNServerConnect * mConnect;
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // PNCheckFeedback
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class PNCheckFeedback : public ConnectManager::TimerListener
    {
    public:
        PNCheckFeedback(APNServerConnect * con) :
            mConnect(con) 
        {
        }
    protected:
        void onTimer(TimeDurMS tick)
        {
            mConnect->checkFeedback();
        }
    private:
        APNServerConnect * mConnect;
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // PNReConnectGateway
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class PNReConnectGateway : public ConnectManager::TimerListener
    {
    public:
        PNReConnectGateway(APNServerConnect * con) :
            mConnect(con) 
        {
        }
    protected:
        void onTimer(TimeDurMS tick)
        {
            mConnect->reconnectGateway();
        }
    private:
        APNServerConnect * mConnect;
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // PNReConnectFeedback
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    class PNReConnectFeedback : public ConnectManager::TimerListener
    {
    public:
        PNReConnectFeedback(APNServerConnect * con) :
            mConnect(con) 
        {
        }
    protected:
        void onTimer(TimeDurMS tick)
        {
            mConnect->reconnectFeedback();
        }
    private:
        APNServerConnect * mConnect;
    };
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // APNServerConnect
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    APNServerConnect::APNServerConnect() :
        mGatewayPrc(0),
        mFeedbackPrc(0)
    {
        mGatewayClient = new AppleGatewayClient();
        mFeedbackClient = new AppleFeedBackClient();
        mRGatewayTimer = new PNCheckGateway(this);
        mRFeedbackTimer = new PNCheckFeedback(this);
        mCGatewayTimer = new PNReConnectGateway(this);
        mCFeedbackTimer = new PNReConnectFeedback(this);
        mSandBox = true;
    }
    //-----------------------------------------------------------------------
    APNServerConnect::~APNServerConnect()
    {
        stop();
        if (mGatewayClient != 0)
        {
            delete mGatewayClient;
            mGatewayClient = 0;
        }

        if (mFeedbackClient != 0)
        {
            delete mFeedbackClient;
            mFeedbackClient = 0;
        }

        if (mRGatewayTimer != 0)
        {
            delete mRGatewayTimer;
            mRGatewayTimer = 0;
        }

        if (mRFeedbackTimer != 0)
        {
            delete mRFeedbackTimer;
            mRFeedbackTimer = 0;
        }

        if (mCGatewayTimer != 0)
        {
            delete mCGatewayTimer;
            mCGatewayTimer = 0;
        }

        if (mCFeedbackTimer != 0)
        {
            delete mCFeedbackTimer;
            mCFeedbackTimer = 0;
        }

        if (mGatewayPrc != 0)
        {
            delete mGatewayPrc;
            mGatewayPrc = 0;
        }

        if (mFeedbackPrc != 0)
        {
            delete mFeedbackPrc;
            mFeedbackPrc = 0;
        }
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::start(ACE_Reactor * react)
    {
        if (mGatewayClient->init(mCertFile, mKeyFile, mKeyPW) == false)
        {
            MlogError("gateway client init ssl failed.");
            return false;
        }

        if (resolvePNIP())
        {
            mGatewayPrc = M_ClientConnect<SSLClientPrc>(react, mGatewayClient, mGatewayIP, mGatewayPort, false);
        }
        else
        {
            return false;
        }

        if (mFeedbackClient->init(mCertFile, mKeyFile, mKeyPW) == false)
        {
            MlogError("feedback client init ssl failed.");
            return false;
        }

        if (resolvePNIP())
        {
            mFeedbackPrc = M_ClientConnect<SSLClientPrc>(react, mFeedbackClient, mFeedbackIP, mFeedbackPort, false);
        }
        else
        {
            return false;
        }
        return true;
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::stop()
    {
        mRGatewayTimer->setEnable(false);
        mRFeedbackTimer->setEnable(false);
        mCGatewayTimer->setEnable(false);
        mCFeedbackTimer->setEnable(false);

        if (mFeedbackClient)
        {
            mFeedbackClient->stop();
        }
        if (mGatewayClient)
        {
            mGatewayClient->stop();
        }
        return true;
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::resolvePNIP()
    {
        Mui32 gatewayidx = 0;
        Mui32 feedbackidx = 0;
        if (mSandBox)
        {
            gatewayidx = 0;
            feedbackidx = 1;
        }
        else
        {
            gatewayidx = 2;
            feedbackidx = 3;
        }

        ACE_INET_Addr gtemp(applepns[gatewayidx].port, applepns[gatewayidx].host);
        if (!gtemp.is_any())
        {
            mGatewayIP = gtemp.get_host_addr();
            mGatewayPort = applepns[gatewayidx].port;
        }
        else
        {
            return false;
        }

        ACE_INET_Addr ftemp(applepns[feedbackidx].port, applepns[feedbackidx].host);
        if (!ftemp.is_any())
        {
            mFeedbackIP = ftemp.get_host_addr();;
            mFeedbackPort = applepns[feedbackidx].port;
        }
        else
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::reconnectGateway()
    {
        if (mGatewayClient)
        {
            mGatewayClient->stop();

            resolvePNIP();

            bool re = M_ClientReConnect<SSLClientPrc>(mGatewayPrc, mGatewayIP, mGatewayPort);
            mRGatewayTimer->setEnable(false);
            mCGatewayTimer->setEnable(true);
            mCGatewayTimer->setInterval(15000);
            return re;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::reconnectFeedback()
    {
        if (mFeedbackClient)
        {
            mFeedbackClient->stop();

            bool re = M_ClientReConnect<SSLClientPrc>(mFeedbackPrc, mFeedbackIP, mFeedbackPort);
            mRFeedbackTimer->setEnable(false);
            mCFeedbackTimer->setEnable(true);
            mCFeedbackTimer->setInterval(15000);
            return re;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void APNServerConnect::checkGateway()
    {
        mCGatewayTimer->setEnable(false);
        if (!mGatewayClient->isSSLConnect())
        {
            mGatewayClient->stop();
        }
    }
    //-----------------------------------------------------------------------
    void APNServerConnect::checkFeedback()
    {
        mCFeedbackTimer->setEnable(false);
        if (!mFeedbackClient->isSSLConnect())
        {
            mFeedbackClient->stop();
        }
    }
    //-----------------------------------------------------------------------
    bool APNServerConnect::sendGateway(const Mui8 * in, MCount size)
    {
        if (mGatewayClient && mGatewayClient->isSSLConnect())
        {
            mGatewayClient->send(in, size);
            return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
}