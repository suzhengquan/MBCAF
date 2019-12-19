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

#include "MdfHttpServerConnect.h"
#include "MdfHttpServerPrc.h"
#include "MdfServerConnect.h"
#include "MdfConfigFile.h"
#include "Mdfversion.h"
#include "MdfConnect.h"
#include "MdfConnectManager.h"

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);
        return 0;
    }
    printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);

    signal(SIGPIPE, SIG_IGN);

    ConfigFile cfgfile("server.conf", "LoginServer");

    String clientAcceptIp;
    String httpAcceptIp;
    String serverAcceptIp;

    Mui32 clientPort = 0;
    Mui32 httpPort = 0;
    Mui32 serverPort = 0;

    cfgfile.getValue("ClientListenIP", clientAcceptIp);
    cfgfile.getValue("ClientPort", clientPort);
    cfgfile.getValue("HttpListenIP", httpAcceptIp);
    cfgfile.getValue("HttpPort", httpPort);

    if (!cfgfile.getValue("ServerListenIP", serverAcceptIp) || !cfgfile.getValue("ServerPort", serverPort))
    {
        return -1;
    }

    StringList strlist;
    StrUtil::split(clientAcceptIp, strlist, ";");
    for (Mui32 i = 0; i < strlist.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_TypeServerConnect(ServerType_Client, reactor, ServerConnect,
            SocketServerPrc, SocketAcceptPrc, strlist[i], clientPort)

        M_Only(ConnectManager)->spawnReactor(4, reactor);
    }

    strlist.clear();
    StrUtil::split(serverAcceptIp, strlist, ";");
    for (Mui32 i = 0; i < strlist.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_TypeServerConnect(ServerType_MsgServer, reactor, ServerConnect, 
            SocketServerPrc, SocketAcceptPrc, strlist[i], serverPort)

        M_Only(ConnectManager)->spawnReactor(1, reactor);
    }
    
    strlist.clear();
    StrUtil::split(httpAcceptIp, strlist, ";");
    for (Mui32 i = 0; i < strlist.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, HttpServerConnect, HttpServerPrc, SocketAcceptPrc, strlist[i], httpPort)

        M_Only(ConnectManager)->spawnReactor(2, reactor);
    }
    
    printf("server start listen on:\nFor client %s:%d\nFor MsgServer: %s:%d\nFor http:%s:%d\n",
        clientAcceptIp, clientPort, serverAcceptIp, serverPort, httpAcceptIp, httpPort);

    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}
