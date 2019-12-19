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
#include "MdfConfigFile.h"
#include "Mdfversion.h"
#include "MdfConnect.h"
#include "MdfConnectManager.h"
#include "MdfSSLClientPrc.h"

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);
        return 0;
    }
    printf("Version:/%s %s %s\n", VERSION, __DATE__, __TIME__);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, DefaultStopSignal);
    signal(SIGTERM, DefaultStopSignal);
    signal(SIGQUIT, DefaultStopSignal);
    
    SSL_library_init();
    SSL_load_error_strings();

    ConfigFile cfgfile("server.conf", "PushServer");

    String serverip;
    String serverpost;
    String certpath;
    String keypath;
    String keypw;
    bool sandbox;

    if (!config_file.getValue("ListenIP", serverip) || !config_file.getValue("ListenPort", serverpost) || 
        !config_file.getValue("CertPath", certpath) || !config_file.getValue("KeyPath", keypath) ||
        !config_file.getValue("SandBox", sandbox) || !config_file.getValue("KeyPassword", keypw))
    {
        return false;
    }


    strlist.clear();
    StrUtil::split(serverAcceptIp, strlist, ";");
    for (Mui32 i = 0; i < strlist.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());

        M_ServerConnect(reactor, ServerConnect, SocketServerPrc, SocketAcceptPrc, serverip[i], serverpost)

        M_Only(ConnectManager)->spawnReactor(2, reactor);
    }

    APNService * apnserver = new APNService(reactor);
    apnserver->setCertFile(cert_path);
    apnserver->setKeyFile(key_path);
    apnserver->setKeyPassword(key_password);
    apnserver->setSandBox(sand_box);

    ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
    if (!apnserver->start(reactor))
    {
        return false;
    }
    M_Only(ConnectManager)->spawnReactor(2, reactor);

    M_Only(ConnectManager)->setTimer(true, 0, 1000);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();

    delete apnserver;
    return 0;
}
