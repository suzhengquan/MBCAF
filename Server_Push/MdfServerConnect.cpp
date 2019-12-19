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

#include "MdfServerConnect.h"
#include "MdfConnectManager.h"
#include "MdfMemStream.h"
#include "MdfMessage.h"
#include "MBCAF.ServerBase.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // ServerConnect
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    ServerConnect::ServerConnect()
    {
        mNotifyMark = 0;
    }
    //-----------------------------------------------------------------------
    ServerConnect::~ServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    ServerIO * ServerConnect::createInstance() const
    {
        return ServerConnect();
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onConnect()
    {
        M_Only(ConnectManager)->addServerConnect(ServerType_Server, this);
        setTimer(true, 0, 3000);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onTimer(TimeDurMS tick)
    {
        if (tick > mReceiveMark + M_Client_Timeout)
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onMessage(Message * msg)
    {
        MdfMessage * temp = static_cast<MdfMessage *>(msg);
        switch (temp->getCommandID())
        {
        case MBCAF::Proto::SBID_Heartbeat:
            prcHeartBeat(temp);
            break;
        case MBCAF::Proto::SBID_UserPushQ:
            prcPushMessage(temp);
            break;
        default:
            stop();
            break;
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::onClose()
    {
        M_Only(ConnectManager)->removeServerConnect(mType, this);
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcPushMessage(MdfMessage * msg)
    {
        MBCAF::ServerBase::UserPushQ proto;
        if (!msg->toProto(&proto))
        {
            stop();
            return;
        }

        String strFlash = proto.flash();
        String strUserData = proto.data();
        APNServer * pClient = M_Only(APNServer);
        if(pClient)
        {
            MBCAF::ServerBase::UserPushA proto2;
            for (uint32_t i = 0; i < proto.user_token_list_size(); ++i)
            {
                MBCAF::Proto::PushResult * push_result = proto2.add_push_result_list();
                MBCAF::Proto::UserTokenInfo user_token = proto.user_token_list(i);
                if (user_token.user_type() != MBCAF::Proto::CT_IOS)
                {
                    push_result->set_user_token(user_token.token());
                    push_result->set_result_code(1);
                    continue;
                }
                ++mNotifyMark;

                Mlog("prcPushMessage, token: %s, push count: %d, push_type:%d, notification id: %u.", user_token.token().c_str(), user_token.push_count(),
                    user_token.push_type(), mNotifyMark);
                AppleGateWayQ remsg0;
                remsg0.setAlterBody(strFlash);
                remsg0.setCustomData(strUserData);
                remsg0.setDeviceToken(user_token.token());
                time_t time_now = 0;
                time(&time_now);
                remsg0.setExpirationDate(time_now + 3600);
                if (user_token.push_type() == PUSH_TYPE_NORMAL)
                {
                    remsg0.setSound(TRUE);
                }
                else
                {
                    remsg0.setSound(FALSE);
                }
                remsg0.setBadge(user_token.push_count());
                remsg0.setNotificationID(mNotifyMark);
                if (remsg0.proto())
                {
                    pClient->sendGateway(remsg0.getBuffer(), remsg0.getSize());
                    push_result->set_result_code(0);
                }
                else
                {
                    push_result->set_result_code(1);
                    Mlog("GateWayMessage failed.");
                }
                push_result->set_user_token(user_token.token());
            }

            MdfMessage remsg;
            remsg.setCommandID(SBMSG(UserPushA));

            if (remsg.setProto(&proto2))
            {
                send(remsg);
            }
            else
            {
                Mlog("UserPushA failed.");
            }
        }
    }
    //-----------------------------------------------------------------------
    void ServerConnect::prcHeartBeat(MdfMessage * msg)
    {
        send(msg);
    }
    //-----------------------------------------------------------------------
}