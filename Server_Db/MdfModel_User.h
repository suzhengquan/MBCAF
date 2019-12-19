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

#ifndef _MDF_MODELUSER_H_
#define _MDF_MODELUSER_H_

#include "MdfPreInclude.h"
#include "MBCAF.Proto.pb.h"

namespace Mdf
{
    void UserLoginValidA(ServerConnect * connect, MdfMessage * msg);

    void UserInfoListA(ServerConnect * connect, MdfMessage * msg);

    void VaryUserInfoListA(ServerConnect * connect, MdfMessage * msg);

    void SignInfoSA(ServerConnect * connect, MdfMessage * msg);

    void PushShieldSA(ServerConnect * connect, MdfMessage * msg);

    void PushShieldA(ServerConnect * connect, MdfMessage * msg);

    void VaryDepartListA(ServerConnect * connect, MdfMessage * msg);

    typedef int (LoginPrc)(const String & name, const String & pw, MBCAF::Proto::UserInfo & user);

    class UserInfoBase
    {
    public:
        UserInfoBase & operator=(const UserInfoBase & o)
        {
            if(this != &o)
            {
                mID = o.mID;
                mSex = o.mSex;
                mState = o.mState;
                mDepartmentID = o.mDepartmentID;
                mNickName = o.mNickName;
                mDomain = o.mDomain;
                mName = o.mName;
                mTel = o.mTel;
                mEmail = o.mEmail;
                mAvatarFile = o.mAvatarFile;
                mSignInfo = o.mSignInfo;
            }
            return *this;
        }
        uint32_t mID;
        uint8_t mSex;
        uint8_t mState;
        uint32_t mDepartmentID;
        String mNickName;
        String mDomain;
        String mName;
        String mTel;
        String mEmail;
        String mAvatarFile;
        String mSignInfo;
    } ;

    class Model_User
    {
    public:
        Model_User();
        ~Model_User();
        void getChangedId(uint32_t & lasttime, list<uint32_t> & idlist);
        void getUsers(const list<uint32_t> & lsIds, list<MBCAF::Proto::UserInfo> & infolist);
        bool getUser(uint32_t userid, UserInfoBase & base);

        bool updateUser(UserInfoBase & base);
        bool insertUser(UserInfoBase & base);
        void clearUserCounter(uint32_t userid, uint32_t sessionid, MBCAF::Proto::SessionType type);
        void setCallReport(uint32_t userid, uint32_t sessionid, MBCAF::Proto::ClientType type);

        bool updateUserSignInfo(uint32_t userid, const String & str);
        bool getUserSingInfo(uint32_t userid, String * str);
        bool updatePushShield(uint32_t userid, uint32_t shield);
        bool getPushShield(uint32_t userid, uint32_t * shield);

        LoginPrc * getLoginPrc() const;
    private:
        LoginPrc * mPrc;
    };
    M_SingletonDef(Model_User);
}
#endif
