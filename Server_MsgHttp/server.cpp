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
#include "MdfConnectManager.h"
#include "MdfRouteClientConnect.h"
#include "MdfDatabaseClientConnect.h"
#include "MdfServerIOPrc.h"
#include "MdfServerConnect.h"
#include "MdfHttpQuery.h"
#include "MdfVersion.h"
#include "MdfConnect.h"
#include "MdfStrUtil.h"

#define DEFAULT_CONCURRENT_DB_CONN_CNT  2

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);
        return 0;
    }
    
    signal(SIGPIPE, SIG_IGN);

    ConfigFile cfgfile("server.conf", "HttpMsgServer");
    String listenIP;
    uint16_t listenPort;

    if (!cfgfile.getValue("ListenIP", listenIP) || !cfgfile.getValue("ListenPort", listenPort))
    {
        return -1;
    }

    ConnectInfoList odbserverlist;
    M_ReadConfig(&cfgfile, odbserverlist, "DBServerIP", "DBServerPort");
    
    ConnectInfoList routeserverlist;
    M_ReadConfig(&cfgfile, routeserverlist, "RouteServerIP", "RouteServerPort");

    Mui32 conperdb = DEFAULT_CONCURRENT_DB_CONN_CNT;
    Mui32 dbconncnt = odbserverlist.size() * DEFAULT_CONCURRENT_DB_CONN_CNT;
    if (cfgfile.getValue("ConcurrentDBConnCnt", conperdb))
    {
        dbconncnt = odbserverlist.size() * conperdb;
    }

    ConnectInfoList dbserverlist;
    if (dbconncnt > 0) 
    {
        for (Mui32 i = 0; i < dbconncnt; i++) 
        {
            ConnectInfo * temp = new ConnectInfo(i);
            temp->mServerIP = odbserverlist[i / conperdb].mServerIP.c_str();
            temp->mServerPort = odbserverlist[i / conperdb].mServerPort;
            dbserverlist.push_back(temp);
        }
    }
    
    StringList listenIPList;
    StrUtil::split(listenIP, listenIPList, ';');
    for (Mui32 i = 0; i < listenIPList.size(); i++) 
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, ServerConnect, ServerPrc, SocketAcceptPrc, listenIPList[i], listenPort)

        M_Only(ConnectManager)->spawnReactor(2, reactor);
    }

    printf("server start listen on: %s:%d\n", listenIP, listenPort);
    
    if (odbserverlist.size() > 0) 
    {
        setupDatabaseConnect(dbserverlist, conperdb);
    }

    if (routeserverlist.size() > 0) 
    {
        setupRouteConnect(routeserverlist);
    }

    M_Only(ConnectManager)->setTimer(true, 0, 1200);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}
