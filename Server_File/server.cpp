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

#include "MdfVersion.h"
#include "MdfConfigFile.h"
#include "MBCAF.Proto.pb.h"
#include "MdfServerConnect.h"
#include "MdfMsgServerConnect.h"
#include "MdfConnectManager.h"

void MdfStopSignal(int signo)
{
	M_OnlyClose(TransferManager);
    DefaultStopSignal(signo);
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
	signal(SIGINT, MdfStopSignal);
	signal(SIGTERM, MdfStopSignal);
	signal(SIGQUIT, MdfStopSignal);
    
    ConfigFile config_file("fileserver.conf", "FileServer");

    String fileIP;
    String clientIP;
    String msgServerIP;
    uint16_t clientPort;
    uint16_t msgServerPort;
    String taskTimeOut;
    
    config_file.getValue("FileServerIP", fileIP);
    config_file.getValue("TaskTimeout", taskTimeOut);

    if (!config_file.getValue("ClientListenIP", clientIP) || 
        !config_file.getValue("ClientListenPort", clientPort) ||
        !config_file.getValue("MsgServerListenIP", msgServerIP) ||
        !config_file.getValue("MsgServerListenPort", msgServerPort))
    {
        return -1;
    }
    Mui32 timeout = atoi(taskTimeOut);
    M_Only(TransferManager)->setTimeOut(timeout);

    StringList fileIPlist;
    StrUtil::split(fileIP, fileIPlist, ';');
    std::list<MBCAF::Proto::IPAddress> q;
    for (Mui32 i = 0; i < fileIPlist.size(); i++) 
    {
        M_Only(TransferManager)->addServer(fileIPlist[i], clientPort);
    }

    StringList clientIPlist;
    StrUtil::split(clientIP, clientIPlist, ';');
    for (Mui32 i = 0; i < clientIPlist.size(); i++) 
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, ServerConnect, SocketServerPrc, SocketAcceptPrc, clientIPlist[i], clientPort);

        M_Only(ConnectManager)->spawnReactor(2, reactor);
    }

    ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
    M_ServerConnect(reactor, MsgServerConnect, SocketServerPrc, SocketAcceptPrc, msgServerIP, msgServerPort);
    M_Only(ConnectManager)->spawnReactor(1, reactor);

    M_Only(ConnectManager)->setTimer(true, 0, 5000);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}