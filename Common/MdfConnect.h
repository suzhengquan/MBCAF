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

#ifndef _MDF_Connect_H_
#define _MDF_Connect_H_

#include "MdfPreInclude.h"
#include "MdfConnectManager.h"
#include "MdfSocketClientPrc.h"

namespace Mdf
{
#define M_ServerConnect(Reactobj, ioPrc, serverPrc, acceptPrc, ip, port)            \
    ServerIO * msgprc = new ioPrc(Reactobj);                                        \
    serverPrc * serverPrc = new serverPrc(msgprc, Reactobj);                        \
    acceptPrc * accept = new acceptPrc(serverPrc, ip, port, Reactobj);              \
    if (accept->open() == -1)                                                       \
    {                                                                               \
        delete accept;                                                              \
        MlogErrorReturn((ACE_TEXT("%N:%l: Failed to open accept handler. Exiting.\n")), -1);\
    }

#define M_TypeServerConnect(type, Reactobj, ioPrc, serverPrc, acceptPrc, ip, port)  \
    ServerIO * msgprc = new ioPrc(Reactobj, type);                                  \
    serverPrc * serverPrc = new serverPrc(msgprc, Reactobj);                        \
    acceptPrc * accept = new acceptPrc(serverPrc, ip, port, Reactobj);              \
    if (accept->open() == -1)                                                       \
    {                                                                               \
        delete accept;                                                              \
        MlogErrorReturn((ACE_TEXT("%N:%l: Failed to open accept handler. Exiting.\n")), -1); \
    }

    template <typename Prc> SocketClientPrc *
        M_ClientConnect(ACE_Reactor * react, ClientIO * msgprc, const String & addr, Mui16 port, bool autoDestroy = true)
    {
        SocketClientPrc * prc = new Prc(msgprc, react);
        prc->setAutoDestroy(autoDestroy);
        ACE_Connector<Prc, ACE_SOCK_CONNECTOR> connector;
        ACE_INET_Addr serIP(port, addr.c_str());
        if (connector.connect(prc, serIP) == -1)
        {
            if (autoDestroy)
            {
                delete prc;
                return 0;
            }
        }
        return prc;
    }

    template <typename Prc> bool
        M_ClientReConnect(SocketClientPrc * prc, const String & addr, Mui16 port)
    {
        ACE_Connector<Prc, ACE_SOCK_CONNECTOR> connector;
        ACE_INET_Addr serIP(port, addr.c_str());
        if (connector.connect(prc, serIP) == -1)
        {
            if(prc->isAutoDestroy())
                delete prc;
            return false;
        }
        return true;
    }

    template <typename Prc, typename Base> void 
        M_ClientConnect(ACE_Reactor * react, const ConnectInfoList & infoList)
    {
        ConnectInfoList::const_iterator i, iend = infoList.end();
        for (i = infoList.begin(); i != iend; ++i)
        {
            infoList[i].mClientIO = new Base();
            infoList[i].mReactor = react;
            infoList[i].mConnect = new Prc(mClientIO, react);
            infoList[i].mCurrentCount = 0;
            infoList[i].mRetryCount = M_ReConnect_Min / 2;

            ACE_INET_Addr serIP(infoList[i].mServerPort, infoList[i].mServerIP.c_str());
            ACE_Connector<Prc, ACE_SOCK_CONNECTOR> connector;
            if (connector.connect(infoList[i].mConnect, serIP) == -1)
            {
                if (infoList[i].mConnect->isAutoDestroy())
                {
                    delete infoList[i].mConnect;
                    infoList[i].mConnect = 0;
                }
                mConnectState = false;
            }
        }
    }

    template <typename Prc> 
        void M_ClientReConnect(const ConnectInfoList & infoList)
    {
        for (uint32_t i = 0; i < infoList.size(); ++i)
        {
            infoList[i].mCurrentCount++;
            if (infoList[i].mCurrentCount < infoList[i].mRetryCount)
            {
                if (!infoList[i].mConnect)
                {
                    infoList[i].mConnect = new Prc(infoList[i].mClientIO, infoList[i].mReactor);
                }
                ACE_INET_Addr serIP(infoList[i].mServerPort, infoList[i].mServerIP.c_str());
                ACE_Connector<Prc, ACE_SOCK_CONNECTOR> connector;
                if (connector.connect(infoList[i].mConnect, serIP) == -1)
                {
                    if (infoList[i].mConnect->isAutoDestroy())
                    {
                        delete infoList[i].mConnect;
                        infoList[i].mConnect = 0;
                    }
                }
            }
        }
    }

    template <class P> void M_ClientReset(const ConnectInfoList & infoList, Mui32 idx)
    {
        if(idx < infoList.size())
        {
            infoList[idx].mConfirmTime = 0;
            infoList[idx].mCurrentCount = 0;
            infoList[idx].mRetryCount *= 2;
            if(infoList[idx].mRetryCount > M_ReConnect_Max)
            {
                infoList[idx].mRetryCount = M_ReConnect_Min;
            }
        }
    }
}
#endif
