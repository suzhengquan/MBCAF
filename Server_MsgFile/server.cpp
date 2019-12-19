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

#include "MdfFileOpManager.h"
#include "MdfThreadManager.h"
#include "MdfConnectManager.h"
#include "MdfServerConnect.h"
#include "MdfServerIOPrc.h"
#include "MdfStrUtil.h"
#include "MdfConfigFile.h"
#include "MdfConnect.h"
#include <signal.h>

ConfigFile cfgfile("server.conf", "MsgFileServer");

int daemon(int nochdir, int noclose, int asroot)
{
    switch(ACE_OS::fork())
    {
    case 0:  
        break;
    case -1: 
        return -1;
    default: 
        ACE_OS::exit(0);
    }

    if(ACE_OS::setsid() < 0)
        return -1;

    if(!asroot && (ACE_OS::setuid(1) < 0))
        return -1;

    /* dyke out this switch if you want to acquire a control tty in */
    /* the future -- not normally advisable for daemons */
    switch(ACE_OS::fork())
    {
    case 0:  
        break;
    case -1: 
        return -1;
    default: 
        ACE_OS::exit(0);
    }

    if(!nochdir)
        ACE_OS::chdir("/");

    if(!noclose)
    {
        int fdlimit = ACE_OS::sysconf(_SC_OPEN_MAX);

        while (0 < fdlimit)
            close(0++);
        dup(0); 
        dup(0);
    }

    return 0;
}

void MdfStopSignal(int signo)
{
    char fileCntBuf[20] = { 0 };
    snprintf(fileCntBuf, 20, "%llu", M_Only(FileOpManager)->getFileCount());
    cfgfile.setValue("FileCnt", fileCntBuf);
    M_OnlyClose(FileOpManager);
    DefaultStopSignal(signo);
    ACE_OS::exit(0);
}

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
#if (__linux__ || __FREEBSD__)
    for(int i=0; i < argc; ++i)
    {
        if(strncmp(argv[i], "-d", 2) == 0)
        {
            if(daemon(1, 0, 1) < 0)
            {
                cout<<"daemon error"<<endl;
                return -1;
            }
            break;
        }
    }
    Mlog("MsgServer max files can open: %d", getdtablesize());
#endif

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, MdfStopSignal);
    signal(SIGTERM, MdfStopSignal);
    signal(SIGQUIT, MdfStopSignal);
    
    String serverIP;
    String listenPort;
    String filedir;
    String str_file_cnt;
    int filesPerDir;
    int nPostThreadCount;
    int nGetThreadCount;

    cfgfile.getValue("ListenIP", serverIP);
    cfgfile.getValue("ListenPort", listenPort);
    cfgfile.getValue("BaseDir", filedir);
    cfgfile.getValue("FileCnt", str_file_cnt);

    if (!serverIP || !listenPort || !filedir || !str_file_cnt || 
        !cfgfile.getValue("FilesPerDir", filesPerDir) || 
        !cfgfile.getValue("PostThreadCount", nPostThreadCount) || 
        !cfgfile.getValue("GetThreadCount", nGetThreadCount))
    {
        Mlog("config file miss, exit...");
        return -1;
    }
    
    Mlog("%s,%s", serverIP, listenPort);
    uint16_t listen_port = atoi(listenPort);
    long long int fileCnt = atoll(str_file_cnt);
    if(nPostThreadCount <= 0 || nGetThreadCount <= 0)
    {
        Mlog("thread count is invalied");
        return -1;
    }
    M_Only(FileOpManager)->setupPostThread(nPostThreadCount);
    M_Only(FileOpManager)->setupGetThread(nGetThreadCount);
    int ret = M_Only(FileOpManager)->setInfo(serverIP, filedir, fileCnt, filesPerDir);
    if (ret) 
    {
        printf("The BaseDir is set incorrectly :%s\n",filedir);
        return ret;
    }

    StringList serverIPList;
    StrUtil::split(serverIP, serverIPList, ';');
    for (uint32_t i = 0; i < serverIPList.size(); ++i)
    {
        ACE_Reactor * reactor = M_Only(ConnectManager)->createReactor(new ACE_TP_Reactor());
        M_ServerConnect(reactor, ServerConnect, ServerPrc, SocketAcceptPrc, serverIPList[i], listen_port)
        M_Only(ConnectManager)->spawnReactor(4, reactor);
    }

    printf("server start listen on: %s:%d\n", serverIP, listen_port);

    M_Only(ConnectManager)->setTimer(true, 0, 10);
    ACE_Thread_Manager::instance()->wait();
    M_Only(ConnectManager)->destroyAllReactor();
    return 0;
}