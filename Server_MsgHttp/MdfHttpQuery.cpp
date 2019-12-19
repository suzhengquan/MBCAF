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

#include "MdfHttpQuery.h"
#include "MdfRouteClientConnect.h"
#include "MdfDataBaseClientConnect.h"
#include "MdfServerConnect.h"
#include "MdfRingBuffer.h"
#include "MdfStrUtil.h"
#include "MBCAF.MsgServer.pb.h"
#include "json/json.h"

#define HTTP_HEADER "HTTP/1.1 200 OK\r\n"\
        "Cache-Control:no-cache\r\n"\
        "Connection:close\r\n"\
        "Content-Length:%d\r\n"\
        "Content-Type:application/javascript\r\n\r\n%s(%s)"

#define HTTP_QUEYR_HEADER "HTTP/1.1 200 OK\r\n"\
        "Cache-Control:no-cache\r\n"\
        "Connection:close\r\n"\
        "Content-Length:%d\r\n"\
        "Content-Type:text/html;charset=utf-8\r\n\r\n%s"

#define MAX_BUF_SIZE 819200

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    void HttpQuery::genResult(RingBuffer & out, Mui32 code, const char * msg)
    {
        Json::Value json_obj;
        out.enlarge(MAX_BUF_SIZE);
        json_obj["error_code"] = code;
        json_obj["error_msg"] = msg;
        String json_str = json_obj.toStyledString();
        Mui32 content_len = json_str.size();

        int i = snprintf(out.getBuffer(), MAX_BUF_SIZE, HTTP_QUEYR_HEADER, content_len, json_str.c_str());

        if (i > 0)
            out.writeSkip(i);
    }
    //-----------------------------------------------------------------------
    void HttpQuery::genCreateGroupResult(RingBuffer & out, Mui32 code, const char * msg, Mui32 group_id)
    {
        Json::Value json_obj;
        out.enlarge(MAX_BUF_SIZE);
        json_obj["error_code"] = code;
        json_obj["error_msg"] = msg;
        json_obj["group_id"] = group_id;
        String json_str = json_obj.toStyledString();
        Mui32 content_len = json_str.size();

        int i = snprintf(out.getBuffer(), MAX_BUF_SIZE, HTTP_QUEYR_HEADER, content_len, json_str.c_str());

        if (i > 0)
            out.writeSkip(i);
    }
    //-----------------------------------------------------------------------
    void HttpQuery::genUserInfoListResult(RingBuffer & out, Mui32 result, const std::list<MBCAF::Proto::UserInfo> & user_list)
    {
        Json::Value json_obj;
        Json::Value user_info_array;
        out.enlarge(MAX_BUF_SIZE);
        json_obj["error_code"] = result;
        json_obj["error_msg"] = "成功";
        if (user_list.size() > 0)
        {
            for(auto user_info : user_list)
            {
                Json::Value user_info_obj;
                user_info_obj.append(user_info.user_nick_name());
                user_info_obj.append(user_info.user_id());
                user_info_array.append(user_info_obj);
            }
            json_obj["user_info_list"] = user_info_array;
        }
        String json_str = json_obj.toStyledString();
        Mui32 content_len = json_str.size();

        int i = snprintf(out.getBuffer(), MAX_BUF_SIZE, HTTP_QUEYR_HEADER, content_len, json_str.c_str());

        if (i > 0)
            out.writeSkip(i);
    }
    //-----------------------------------------------------------------------
    static Mui32 g_total_query = 0;
    static Mui32 g_last_year = 0;
    static Mui32 g_last_month = 0;
    static Mui32 g_last_mday = 0;
    //-----------------------------------------------------------------------
    void http_query_timer_callback(void * callback_data, uint8_t msg, Mui32 handle, void * pParam)
    {
        struct tm * tm;
        time_t currTime;

        time(&currTime);
        tm = localtime(&currTime);

        Mui32 year = tm->tm_year + 1900;
        Mui32 mon = tm->tm_mon + 1;
        Mui32 mday = tm->tm_mday;
        if (year != g_last_year || mon != g_last_month || mday != g_last_mday)
        {
            Mlog("a new day begin, g_total_query=%u ", g_total_query);
            g_total_query = 0;
            g_last_year = year;
            g_last_month = mon;
            g_last_mday = mday;
        }
    }
    //-----------------------------------------------------------------------
    M_SingletonImpl(HttpQuery);
    //-----------------------------------------------------------------------
    HttpQuery::HttpQuery()
    {
        netlib_register_timer(http_query_timer_callback, NULL, 1000);
    }
    //-----------------------------------------------------------------------
    HttpQuery::~HttpQuery() 
    {
    }
    //-----------------------------------------------------------------------
    void HttpQuery::query(String & url, String & post_data, ServerConnect * sconn)
    {
        ++g_total_query;

        Mlog("query, url=%s, content=%s ", url.c_str(), post_data.c_str());

        Json::Reader reader;
        Json::Value value;
        Json::Value root;

        if (!reader.parse(post_data, value))
        {
            Mlog("json parse failed, post_data=%s ", post_data.c_str());
            sconn->stop();
            return;
        }

        String strErrorMsg;
        String strAppKey;
        HttpErrorType nRet = HET_OK;
        try
        {
            String strInterface(url.c_str() + strlen("/query/"));
            strAppKey = value["app_key"].asString();
            String strIp = sconn->getIP();
            Mui32 nUserId = value["req_user_id"].asUInt();
            nRet = checkAuth(strAppKey, nUserId, strInterface, strIp);
        }
        catch (std::runtime_error msg)
        {
            nRet = HET_Interface;
        }

        if (HET_OK != nRet)
        {
            if (nRet < HET_Max)
            {
                root["error_code"] = nRet;
                root["error_msg"] = HTTP_ERROR_MSG[nRet];
            }
            else
            {
                root["error_code"] = -1;
                root["error_msg"] = "未知错误";
            }
            String strResponse = root.toStyledString();
            sconn->send((void*)strResponse.c_str(), strResponse.length());
            return;
        }

        // process post request with post content
        if (strcmp(url.c_str(), "/query/Group") == 0)
        {
            queryGroup(strAppKey, value, sconn);
        }
        else if (strcmp(url.c_str(), "/query/VaryMember") == 0)
        {
            queryVaryMember(strAppKey, value, sconn);
        }
        else
        {
            Mlog("url not support ");
            sconn->stop();
            return;
        }
    }
    //-----------------------------------------------------------------------
    void HttpQuery::queryGroup(const String & strAppKey, Json::Value & post_json_obj, ServerConnect * sconn)
    {
        DataBaseClientConnect * conn = DataBaseClientConnect::getPrimaryConnect();
        if (!conn)
        {
            Mlog("no connection to DBProxy ");
            RingBuffer temp;
            genResult(temp, HET_ServerError, HTTP_ERROR_MSG[9].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["req_user_id"].isNull())
        {
            Mlog("no user id ");
            RingBuffer temp;
            genResult(temp, HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["group_name"].isNull())
        {
            Mlog("no group name ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["group_type"].isNull())
        {
            Mlog("no group type ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["group_avatar"].isNull())
        {
            Mlog("no group avatar ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["user_id_list"].isNull())
        {
            Mlog("no user list ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        try
        {
            Mui32 userid = post_json_obj["req_user_id"].asUInt();
            String group_name = post_json_obj["group_name"].asString();
            Mui32 group_type = post_json_obj["group_type"].asUInt();
            String group_avatar = post_json_obj["group_avatar"].asString();
            Mui32 user_cnt = post_json_obj["user_id_list"].size();
            Mlog("QueryCreateGroup, user_id: %u, group_name: %s, group_type: %u, user_cnt: %u. ",
                userid, group_name.c_str(), group_type, user_cnt);
            if (!MBCAF::Proto::GroupType_IsValid(group_type))
            {
                Mlog("QueryCreateGroup, unvalid group_type");
                RingBuffer temp;
                genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
                sconn->send(temp.getBuffer(), temp.getWriteSize());
                sconn->stop();
                return;
            }

            HandleExtData extdata(sconn->getID());
            MBCAF::MsgServer::GroupCreateQ msg;
            msg.set_user_id(0);
            msg.set_group_name(group_name);
            msg.set_group_avatar(group_avatar);
            msg.set_group_type((::MBCAF::Proto::GroupType)group_type);
            for (Mui32 i = 0; i < user_cnt; i++)
            {
                Mui32 member_id = post_json_obj["user_id_list"][i].asUInt();
                msg.add_member_id_list(member_id);
            }
            msg.set_attach_data(extdata.getBuffer(), extdata.getSize());

            MdfMessage pdu;
            pdu.setProto(&msg);
            pdu.setCommandID(MSMSG(CreateGroupQ));
            conn->send(&pdu);

        }
        catch(std::runtime_error msg)
        {
            Mlog("parse json data failed.");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
        }
    }
    //-----------------------------------------------------------------------
    void HttpQuery::queryVaryMember(const String & strAppKey, Json::Value & post_json_obj, ServerConnect * sconn)
    {
        DataBaseClientConnect * conn = DataBaseClientConnect::getPrimaryConnect();
        if (!conn)
        {
            Mlog("no connection to RouteServConn ");
            RingBuffer temp;
            genResult(HET_ServerError, HTTP_ERROR_MSG[9].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }
        if (post_json_obj["req_user_id"].isNull())
        {
            Mlog("no user id ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["group_id"].isNull())
        {
            Mlog("no group id ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["modify_type"].isNull())
        {
            Mlog("no modify_type ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        if (post_json_obj["user_id_list"].isNull())
        {
            Mlog("no user list ");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
            return;
        }

        try
        {
            Mui32 userid = post_json_obj["req_user_id"].asUInt();
            Mui32 group_id = post_json_obj["group_id"].asUInt();
            Mui32 modify_type = post_json_obj["modify_type"].asUInt();
            Mui32 user_cnt = post_json_obj["user_id_list"].size();
            Mlog("QueryChangeMember, user_id: %u, group_id: %u, modify type: %u, user_cnt: %u. ",
                userid, group_id, modify_type, user_cnt);
            if (!MBCAF::Proto::GroupModifyType_IsValid(modify_type))
            {
                Mlog("QueryChangeMember, unvalid modify_type");
                RingBuffer temp;
                genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
                sconn->send(temp.getBuffer(), temp.getWriteSize());
                sconn->stop();
                return;
            }
            HandleExtData extdata(sconn->getID());
            MBCAF::MsgServer::GroupMemberSQ msg;
            msg.set_user_id(0);
            msg.set_change_type((MBCAF::Proto::GroupModifyType)modify_type);
            msg.set_group_id(group_id);
            for(Mui32 i = 0; i < user_cnt; i++)
            {
                Mui32 member_id = post_json_obj["user_id_list"][i].asUInt();
                msg.add_member_id_list(member_id);
            }
            msg.set_attach_data(extdata.getBuffer(), extdata.getSize());
            MdfMessage pdu;
            pdu.setProto(&msg);
            pdu.setCommandID(MSMSG(GroupMemberSQ));
            conn->send(&pdu);
        }
        catch (std::runtime_error msg)
        {
            Mlog("parse json data failed.");
            RingBuffer temp;
            genResult(HET_Parment, HTTP_ERROR_MSG[1].c_str());
            sconn->send(temp.getBuffer(), temp.getWriteSize());
            sconn->stop();
        }
    }
    //-----------------------------------------------------------------------
    HttpErrorType HttpQuery::checkAuth(const String & strAppKey, const Mui32 userId, 
        const String & strInterface, const String & strIp)
    {
        return HET_OK;
    }
    //-----------------------------------------------------------------------
}