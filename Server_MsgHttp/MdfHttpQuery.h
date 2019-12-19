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

#ifndef _MDF_HTTPQUERY_H_
#define _MDF_HTTPQUERY_H_

#include "MdfPreInclude.h"
#include "MBCAF.Proto.pb.h"
#include "json/json.h"

namespace Mdf
{
    enum HttpErrorType
    {
        HET_OK = 0,
        HET_Unknow,
        HET_Parment,
        HET_AppKey,
        HET_Match,
        HET_Permission,
        HET_Interface,
        HET_IP,
        HET_SendType,
        HET_Max,
        HET_ServerError,
        HET_GroupCreate,
        HET_GroupMember,
        HET_Encrypt,
    };

    static String HTTP_ERROR_MSG[] =
    {
        "成功",
        "未知",
        "参数",
        "appKey不存在",
        "appKey与用户不匹配",
        "含有不允许发送的Id",
        "未授权的接口",
        "未授权的IP",
        "非法的发送类型",
        "服务器异常",
        "创建群失败",
        "更改群成员失败",
        "消息加密失败",
    };

    class HttpQuery
    {
    public:
        HttpQuery();
        virtual ~HttpQuery();

        static void query(String & url, String & post_data, ServerConnect * sconn);

        static void genResult(RingBuffer & out, Mui32 code, const char * msg = "");
        static void genCreateGroupResult(RingBuffer & out, Mui32 code, const char * msg, Mui32 group_id);
        static void genUserInfoListResult(RingBuffer & out, Mui32 result, const std::list<MBCAF::Proto::UserInfo> & user_list);
    private:
        static void queryGroup(const String & strAppKey, Json::Value & post_json_obj, ServerConnect * sconn);
        static void queryVaryMember(const String & strAppKey, Json::Value & post_json_obj, ServerConnect * sconn);
        static HttpErrorType checkAuth(const String & strAppKey, const Mui32 userId, const String & strInterface, const String & strIp);
    };

    M_SingletonDef(HttpQuery);
}
#endif
