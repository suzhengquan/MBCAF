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

#ifndef _MDF_MODELGroup_H_
#define _MDF_MODELGroup_H_

#include "MdfPreInclude.h"
#include "MdfModel_Message.h"
#include "MBCAF.Proto.pb.h"

namespace Mdf
{
    void GroupCreateA(ServerConnect * connect, MdfMessage * msg);

    void GroupListA(ServerConnect * connect, MdfMessage * msg);

    void GroupInfoListA(ServerConnect * connect, MdfMessage * msg);

    void GroupMemberSA(ServerConnect * connect, MdfMessage * msg);

    void GroupShieldSA(ServerConnect * connect, MdfMessage * msg);

    void GroupShieldA(ServerConnect * connect, MdfMessage * msg);

    const uint32_t MAX_UNREAD_COUNT = 100;

    class Model_Group
    {
    public:
        virtual ~Model_Group();
    public:
        uint32_t createGroup(uint32_t owner, const String & name, const String & avatar, uint32_t type, set<uint32_t> & oplist);
        bool removeGroup(uint32_t owner, uint32_t gid, list<uint32_t> & usridlist);
        void getUserGroup(uint32_t owner, list<uint32_t> & idlist, uint32_t cnt = 100);
        void getUserGroup(uint32_t owner, list<MBCAF::Proto::GroupVersionInfo> & gverlist, uint32_t type);
        void getGroupInfo(map<uint32_t, MBCAF::Proto::GroupVersionInfo> & verlist, list<MBCAF::Proto::GroupInfo> & infolist);
        bool setPush(uint32_t userid, uint32_t gid, uint32_t type, uint32_t state);
        void getPush(uint32_t gid, list<uint32_t> & usridlist, list<MBCAF::Proto::ShieldStatus> & statelist);
        bool modifyGroupMember(uint32_t userid, uint32_t gid, MBCAF::Proto::GroupModifyType type, set<uint32_t> & setUserId,
            list<uint32_t> & usridlist);
        void getGroupUser(uint32_t gid, list<uint32_t> & usridlist);
        bool isInGroup(uint32_t userid, uint32_t gid);
        void updateGroupChat(uint32_t gid);
        bool isValidateGroupId(uint32_t gid);
        uint32_t getUserJoinTime(uint32_t gid, uint32_t userid);
    private:
        Model_Group();

        bool insertNewGroup(uint32_t userid, const String & name, const String & avatar, uint32_t type, uint32_t usrcnt, uint32_t & groupId);
        bool insertNewMember(uint32_t gid, set<uint32_t> & oplist);
        void getGroupVersion(list<uint32_t>& idlist, list<MBCAF::Proto::GroupVersionInfo> & gverlist, uint32_t type);
        bool hasModifyPermission(uint32_t userid, uint32_t gid, MBCAF::Proto::GroupModifyType type);
        bool addMember(uint32_t gid, set<uint32_t> & oplist, list<uint32_t> & usridlist);
        bool removeMember(uint32_t gid, set<uint32_t> & oplist, list<uint32_t> & usridlist);
        void removeSession(uint32_t gid, const set<uint32_t> & usridlist);

        void fillGroupMember(list<MBCAF::Proto::GroupInfo> & infolist);
    };
    M_SingletonDef(Model_Group);
}
#endif
