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

#ifndef _MESSAGE_MODEL_H_
#define _MESSAGE_MODEL_H_

#include "MdfPreInclude.h"
#include "MdfStrUtil.h"
#include "MdfModel_Audio.h"
#include "MdfModel_Group.h"
#include "MBCAF.Proto.pb.h"

namespace Mdf
{
    void MessageInfoListA(ServerConnect * qconn, MdfMessage * msg);

    void SendMessage(ServerConnect * qconn, MdfMessage * msg);

    void MessageInfoByIdA(ServerConnect * qconn, MdfMessage * msg);

    void MessageRecentA(ServerConnect * qconn, MdfMessage * msg);

    void MessageUnreadCntA(ServerConnect * qconn, MdfMessage * msg);

    void MessageReadA(ServerConnect * qconn, MdfMessage * msg);

    void TrayMsgA(ServerConnect * qconn, MdfMessage * msg);

    void SwitchTrayMsgA(ServerConnect * qconn, MdfMessage * msg);

    class Model_Relation
    {
    public:
        virtual ~Model_Relation();

        uint32_t getRelationId(uint32_t aid, uint32_t bid, bool add);

        bool updateRelation(uint32_t relid, uint32_t time);

        bool removeRelation(uint32_t relid);
    private:
        Model_Relation();
    };

    M_SingletonDef(Model_Relation);

    class Model_Message
    {
    public:
        virtual ~Model_Message();

        void setupEncryptStr(const String & str);

        bool sendMessage(uint32_t rid, uint32_t fromid, uint32_t toid, MBCAF::Proto::MessageType type, uint32_t ctime, uint32_t msgid, String & data);

        bool sendAudioMessage(uint32_t rid, uint32_t fromid, uint32_t toid, MBCAF::Proto::MessageType mtype, uint32_t ctime, uint32_t msgid, const char * data, uint32_t dsize);

        void getMessage(uint32_t userid, uint32_t sessionid, uint32_t msgid, uint32_t cnt, list<MBCAF::Proto::MsgInfo> & infolist);
        
        bool resetMessageID(uint32_t rid);

        uint32_t getNextMessageId(uint32_t rid);
        
        void getUnreadMessage(uint32_t userid, uint32_t & cnt, list<MBCAF::Proto::UnreadInfo> & infolist);
        
        void getLastMsg(uint32_t fromid, uint32_t toid, uint32_t & msgid, String & message, MBCAF::Proto::MessageType & mtype, uint32_t state = 0);
        
        void getUserUnread(uint32_t userid, uint32_t & cnt);
        
        void getMsgByMsgId(uint32_t userid, uint32_t sessionid, const list<uint32_t> & idlist, list<MBCAF::Proto::MsgInfo> & infolist);
    
        static String AudioEncryptStr;
    private:
        Model_Message();
    };
    M_SingletonDef(Model_Message);

    class Model_GroupMessage
    {
    public:
        virtual ~Model_GroupMessage();

        bool sendMessage(uint32_t fromid, uint32_t gid, MBCAF::Proto::MessageType mtype, uint32_t ctime, uint32_t msgid, const String & data);

        bool sendAudioMessage(uint32_t fromid, uint32_t gid, MBCAF::Proto::MessageType mtype, uint32_t ctime, uint32_t msgid, const char * data, uint32_t dsize);

        void getMessage(uint32_t userid, uint32_t gid, uint32_t msgid, uint32_t cnt, list<MBCAF::Proto::MsgInfo> & infolist);

        bool resetMessageID(uint32_t gid);

        uint32_t getNextMessageId(uint32_t gid);

        void getUnreadMessage(uint32_t userid, uint32_t & cnt, list<MBCAF::Proto::UnreadInfo> & infolist);

        void getLastMsg(uint32_t gid, uint32_t & msgid, String & message, MBCAF::Proto::MessageType & type, uint32_t & fromid);

        void getUserUnread(uint32_t userid, uint32_t & cnt);

        void getMsgByMsgId(uint32_t userid, uint32_t gid, const list<uint32_t> & idlist, list<MBCAF::Proto::MsgInfo> & infolist);
    private:
        Model_GroupMessage();
    };
    M_SingletonDef(Model_GroupMessage);
}
#endif
