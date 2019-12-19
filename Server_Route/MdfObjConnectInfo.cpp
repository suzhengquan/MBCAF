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

#include "MdfObjConnectInfo.h"
#include "MBCAF.Proto.pb.h"

using namespace MBCAF::Proto;

namespace Mdf
{
    //-----------------------------------------------------------------------
    ObjConnectInfo::ObjConnectInfo()
    {
    }
    //-----------------------------------------------------------------------
    ObjConnectInfo::~ObjConnectInfo()
    {
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::addLogin(ServerConnect * con) 
    { 
        mLoginList.insert(con);
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::removeLogin(ServerConnect * con)
    { 
        mLoginList.erase(con);
    }
    //-----------------------------------------------------------------------
    bool ObjConnectInfo::isLoginExist(ServerConnect * con) const
    {
        ConnectList::const_iterator it = mLoginList.find(con);
        if (it != mLoginList.end())
        {
            return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::clearAllLogin() 
    { 
        mLoginList.clear(); 
    }
    //-----------------------------------------------------------------------
    const ConnectList & ObjConnectInfo::getLoginList() const 
    { 
        return mLoginList; 
    }
    //-----------------------------------------------------------------------
    MCount ObjConnectInfo::getLoginCount() const
    { 
        return mLoginList.size(); 
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::addLoginType(Mui32 type)
    {
        ObjectTypeList::iterator it = mLoginTypeList.find(type);
        if (it != mLoginTypeList.end())
        {
            it->second += 1;
        }
        else
        {
            
            mLoginTypeList.insert(make_pair(type, 1));
        }
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::removeLoginType(Mui32 type)
    {
        ObjectTypeList::iterator it = mLoginTypeList.find(type);
        if (it != mLoginTypeList.end())
        {
            if (--it->second <= 0)
            {
                mLoginTypeList.erase(type);
            }
        }
    }
    //-----------------------------------------------------------------------
    MCount ObjConnectInfo::getLoginTypeCount(Mui32 type) const
    {
        ObjectTypeList::const_iterator it = mLoginTypeList.find(type);
        if (it != mLoginTypeList.end())
        {
            return it->second;
        }

        return 0;
    }
    //-----------------------------------------------------------------------
    const ObjectTypeList & ObjConnectInfo::getLoginTypeList() const
    {
        return mLoginTypeList;
    }
    //-----------------------------------------------------------------------
    void ObjConnectInfo::clearAllLoginType()
    {
        mLoginTypeList.clear();
    }
    //-----------------------------------------------------------------------
    bool ObjConnectInfo::isPCLogin() const
    {
        ObjectTypeList::const_iterator it, itend = mLoginTypeList.end();
        for(it = mLoginTypeList.begin(); it != itend; ++it)
        {
            if(M_PCLoginCheck(it->first))
            {
                return true;
            }
        }
        return false;
    }
    //-----------------------------------------------------------------------
    bool ObjConnectInfo::isMobileLogin() const
    {
        ObjectTypeList::const_iterator it, itend = mLoginTypeList.end();
        for(it = mLoginTypeList.begin(); it != itend; ++it)
        {
            if (M_MobileLoginCheck(it->first))
            {
                return true;
            }
        }
        return false;
    }
    //-----------------------------------------------------------------------
    bool ObjConnectInfo::isGameLogin() const
    {
    }
    //-----------------------------------------------------------------------
    Mui32 ObjConnectInfo::getState() const
    {
        ObjectTypeList::const_iterator it, itend = mLoginTypeList.end();
        for (it = mLoginTypeList.begin(); it != itend; ++it)
        {
            Mui32 type = it->first;
            if (M_PCLoginCheck(type))
            {
                return OST_Online;
            }
            else if (M_MobileLoginCheck(type))
            {
                return OST_Online;
            }
        }
        return OST_Offline;
    }
    //-----------------------------------------------------------------------
}