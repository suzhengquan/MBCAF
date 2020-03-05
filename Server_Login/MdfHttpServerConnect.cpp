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
#include "json/json.h"

#define HTTP_A_Text "HTTP/1.1 200 OK\r\nConnection:close\r\nContent-Length:%d\r\n"\
                    "Content-Type:text/html;charset=utf-8\r\n\r\n%s"
#define HTTP_A_MaxSize    1024

namespace Mdf
{
    //-----------------------------------------------------------------------
    HttpServerConnect::HttpServerConnect(ACE_Reactor * tor) :
        ServerIO(tor)
    {
    }
    //-----------------------------------------------------------------------
    HttpServerConnect::HttpServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    HttpServerConnect::~HttpServerConnect()
    {
    }
    //-----------------------------------------------------------------------
    ServerIO * HttpServerConnect::createInstance() const
    {
        return HttpServerConnect();
    }
    //-----------------------------------------------------------------------
    void HttpServerConnect::onConnect()
    {
        setTimer(true, 0, 1000);
    }
    //-----------------------------------------------------------------------
    void HttpServerConnect::onMessage(Message * msg)
    {
        mHttpParser.parse(msg->getBuffer(), msg->getSize());

        if(mHttpParser.isComplete()) 
        {
            String url = mHttpParser.getUrl();
            if(strncmp(url.c_str(), "/server", 7) == 0) 
            {
                String temp(url.c_str(), 2, 8);
                String content = mHttpParser.getBodyContent();
                prcServerQ(url, content, strtoi(temp));
            } 
            else 
            {
                Mlog("url unknown, url=%s ", url.c_str());
                stop();
            }
        }
    }
    //-----------------------------------------------------------------------
    void HttpServerConnect::onTimer(TimeDurMS curr_tick)
    {
        if (curr_tick > mReceiveMark + M_HTTP_Timeout) 
        {
            stop();
        }
    }
    //-----------------------------------------------------------------------
    void HttpServerConnect::prcServerQ(String & url, String & post_data, Nui8 type)
    {
        if(M_Only(ConnectManager)->getServerCount(type) == 0)
        {
            Json::Value value;
            value["code"] = 1;
            value["msg"] = "no server";
            String valuestr = value.toStyledString();
            char* szContent = (char *)malloc(HTTP_A_MaxSize);
            snprintf(szContent, HTTP_A_MaxSize, HTTP_A_Text, valuestr.length(), valuestr.c_str());
            send(szContent, strlen(szContent));
            stop();
            free(szContent);
            return ;
        }
    
        ServerInfo * server = M_Only(ConnectManager)->getServer(type, true);
        if (server == 0)
        {
            Json::Value value;
            value["code"] = 2;
            value["msg"] = "server full";
            String valuestr = value.toStyledString();
            char* szContent = (char *)malloc(HTTP_A_MaxSize);
            snprintf(szContent, HTTP_A_MaxSize, HTTP_A_Text, valuestr.length(), valuestr.c_str());
            send(szContent, strlen(szContent));
            stop();
            free(szContent);
            return;
        } 
        else 
        {
            Json::Value value;
            value["code"] = 0;
            value["msg"] = "server ok";
            value["priorIP"] = String(server->mIP);
            value["backupIP"] = String(server->mIP2);
            value["msfsPrior"] = MsgFileSystemAdr;
            value["msfsBackup"] = MsgFileSystemAdr;
            value["discovery"] = DiscoveryAdr;
            value["port"] = itostr(server->mPort);
            String valuestr = value.toStyledString();
            char * szContent = (char *)malloc(HTTP_A_MaxSize);
            snprintf(szContent, HTTP_A_MaxSize, HTTP_A_Text, valuestr.length(), valuestr.c_str());
            send(szContent, strlen(szContent));
            stop();
            free(szContent);
            return;
        }
    }
    //-----------------------------------------------------------------------
}