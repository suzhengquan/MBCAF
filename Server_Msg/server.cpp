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

#include "MdfEncrypt.h"
#include "MdfConfigFile.h"
#include "MdfLoginClientConnect.h"
#include "MdfRouteClientConnect.h"
#include "MdfDataBaseClientConnect.h"
#include "MdfPushClientConnect.h"
#include "MdfFileClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfConnectManager.h"

#define DBDefaultConnectCnt  10

static void stop(int sig_no)
{
    shutdownFileConnect();

    shutdownDataBaseConnect();

    shutdownLoginConnect();

    shutdownRouteConnect();

    shutdownPushConnect();
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

    Mlog("MsgServer max files can open: %d ", getdtablesize());

    ConfigFile cfgfile("server.conf");

    String listenip;
    Mui16 listenport;
    String primaryip;
    String slaveip;
    Mui32 maxconn;
    String AESkey;
    ConnectInfoList dbserlist;
    ConnectInfoList loginserlist;
    ConnectInfoList routeserlist;
    ConnectInfoList pushserlist;
    ConnectInfoList fileserlist;

    cfgfile.getValue("IpAddr2", slaveip);
    cfgfile.getValue("MaxConnCnt", maxconn);
    cfgfile.getValue("aesKey", AESkey);

    M_ReadConfig(&cfgfile, dbserlist, "DBServerIP", "DBServerPort");
    M_ReadConfig(&cfgfile, loginserlist, "LoginServerIP", "LoginServerPort");
    M_ReadConfig(&cfgfile, routeserlist, "RouteServerIP", "RouteServerPort");
    M_ReadConfig(&cfgfile, pushserlist, "PushServerIP", "PushServerPort");
    M_ReadConfig(&cfgfile, fileserlist, "FileServerIP", "FileServerPort");

    if (!AESkey || strlen(AESkey) != 32)
    {
        Mlog("aes key is invalied");
        return -1;
    }

    if (dbserlist.size() < 2)
    {
        Mlog("DBServerIP need 2 instance at lest ");
        return 1;
    }

    Mui32 dbconncnt = DBDefaultConnectCnt;
    Mui32 dstdbconncnt = dbserlist.size() * DBDefaultConnectCnt;
    if(cfgfile.getValue("ConcurrentDBConnCnt", dbconncnt))
    {
        dbconncnt = atoi(concurrent_db_conn);
        dstdbconncnt = dbserlist.size() * dbconncnt;
    }

    ConnectInfoList dstdbserverlist;;
    for(Mui32 i = 0; i < dstdbconncnt; i++)
    {
        ConnectInfo * info = new ConnectInfo(i);
        dstdbserverlist[i].mServerIP = dbserlist[i / dbconncnt].mServerIP.c_str();
        dstdbserverlist[i].mServerPort = dbserlist[i / dbconncnt].mServerPort;
        dstdbserverlist.push_back(info);
    }

    if (!cfgfile.getValue("ListenIP", listenip) || 
        !cfgfile.getValue("ListenPort", listenport) || 
        !cfgfile.getValue("IpAddr1", primaryip))
    {
        Mlog("config file miss, exit... ");
        return -1;
    }

    if (!slaveip)
    {
        slaveip = primaryip;
    }

    StringList listeniplist;
    StrUtil::split(listenip, listeniplist, ';');
    for (Mui32 i = 0; i < listeniplist.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, ServerConnect, SocketServerPrc, SocketAcceptPrc, listeniplist[i], listenport)

        M_Only(ConnectManager)->spawnReactor(4, reactor);
    }

    printf("server start listen on: %s:%d\n", listenip, listenport);

    setupSignal();
    setupFileConnect(fileserlist);
    setupDataBaseConnect(dstdbserverlist, dbconncnt, AESkey);
    setupLoginConnect(loginserlist, primaryip, slaveip, listenport, maxconn);
    setupRouteConnect(routeserlist);
    setupPushConnect(pushserlist);

    M_Only(ConnectManager)->setTimer(true, 0, 1000);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}