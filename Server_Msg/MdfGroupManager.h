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

#ifndef _MDF_GROUPManager_H_
#define _MDF_GROUPManager_H_

#include "MdfPreInclude.h"

namespace Mdf
{
    class ServerConnect;

    class GroupManager
    {
    public:
        virtual ~GroupManager() {}

        void GroupListQ(ServerConnect * conn, MdfMessage * msg);
        void GroupInfoListQ(ServerConnect * conn, MdfMessage * msg);
        void GroupCreateQ(ServerConnect * conn, MdfMessage * msg);
        void GroupMemberSQ(ServerConnect * conn, MdfMessage * msg);
        void GroupShieldSQ(ServerConnect * conn, MdfMessage * msg);
    
        void prcGroupListA(MdfMessage * msg);
        void prcGroupInfoA(MdfMessage * msg);
        void prcGroupMessage(MdfMessage * msg);
        void prcGroupMessageBroadcast(MdfMessage * msg);
        void prcCreateGroupA(MdfMessage * msg);
        void prcGroupMemberSA(MdfMessage * msg);
        void prcGroupMemberNotify(MdfMessage * msg);
        void prcGroupShieldSA(MdfMessage * msg);
        void prcGroupShieldA(MdfMessage * msg);
    private:
        GroupManager() {}
    protected:
        void sendToUser(Mui32 userid, MdfMessage * msg, ServerConnect * qconn = NULL);
    };
    M_SingletonDef(DataSyncManager);
}
#endif
