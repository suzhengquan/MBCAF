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

#include "MdfConfigFile.h"
#include "MdfVersion.h"
#include "MdfDataSyncManager.h"
#include "MdfConnectManager.h"
#include "MdfDatabaseManager.h"
#include "MdfMembaseManager.h"
#include "MdfServerConnect.h"
#include "MdfServerIOPrc.h"
#include "MdfModel_Audio.h"
#include "MdfModel_Message.h"
#include "MdfModel_Session.h"
#include "MdfModel_User.h"
#include "MdfModel_Group.h"
#include "MdfModel_File.h"

using namespace MBCAF::Proto;

static void signalTerm(int sig_no)
{
    Mlog("receive SIGTERM, prepare for exit");
    MdfMessage msg;

    MBCAF::ServerBase::StopReceive proto;
    proto.set_result(0);
    msg.setProto(&proto);
    msg.setCommandID(SBMSG(StopReceive));

    M_Only(ConnectManager)->sendServerConnect(ServerType_Server, &msg);
    M_OnlyClose(DataSyncManager)
    ACE_OS::sleep(ACE_Time_Value(4));
    ACE_OS::exit(0);
}

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);
        return 0;
    }
    printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, signalTerm);
    srand(time(NULL));

    M_Only(MembaseManager);
    M_Only(DatabaseManager);
    M_Only(DataSyncManager);
    M_Only(Model_Audio);
    M_Only(Model_Message);
    M_Only(Model_Group);
    M_Only(Model_GroupMessage);
    M_Only(Model_Session);
    M_Only(Model_Relation);
    M_Only(Model_User);
    M_Only(Model_File);

    ConfigFile config_file("server.conf", "DBServer");

    String serverIP;
    String strFileSite;
    String strAesKey;
    Mui32 serverPort;
    Mui32 threadCnt;

    if (!config_file.getValue("ListenIP", serverIP)    || 
        !config_file.getValue("ListenPort", serverPort) ||
        !config_file.getValue("ThreadNum", threadCnt) ||
        !config_file.getValue("MsfsSite", strFileSite) ||
        !config_file.getValue("aesKey", strAesKey))
    {
        Mlog("error config exit...");
        return -1;
    }

    if (strlen(strAesKey) != 32)
    {
        Mlog("aes key is invalied");
        return -2;
    }
    M_Only(DataSyncManager)->setupMsgThread(threadCnt);
    M_Only(Model_Message)->setupEncryptStr(strAesKey);
    M_Only(Model_Audio)->setUrl(strFileSite);

    StringList serverIPList;
    StrUtil::split(serverIP, serverIPList, ';');
    for(uint32_t i = 0; i < serverIPList.size(); ++i) 
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, ServerConnect, ServerPrc, SocketAcceptPrc, serverIPList[i], serverPort)

        M_Only(ConnectManager)->spawnReactor(4, reactor);
    }

    printf("server start listen on: %s:%d\n", serverIP, serverPort);

    M_Only(ConnectManager)->setTimer(true, 0, 10);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}