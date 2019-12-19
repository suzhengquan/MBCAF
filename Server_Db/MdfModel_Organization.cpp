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

#include "MdfModel_Depart.h"
#include "MdfDatabaseManager.h"

namespace Mdf
{
    //----------------------------------------------------------------
    M_SingletonImpl(Model_Organization);
    //----------------------------------------------------------------
    void Model_Organization::getVaryOrgList(uint32_t & time, list<uint32_t> & idlist)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select id, updated from IMOrg where updated > " + itostr(time);
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    uint32_t id;
                    uint32_t updatetime;
                    resSet->getValue("id", id);
                    resSet->getValue("updated", updatetime);

                    if (time < updatetime)
                    {
                        time = updatetime;
                    }
                    idlist.push_back(id);
                }
                delete resSet;
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave.");
        }
    }
    //----------------------------------------------------------------
    void Model_Organization::getOrgList(const list<uint32_t> & idlist, list<MBCAF::Proto::OrgInfo> & infolist)
    {
        if (idlist.empty())
        {
            Mlog("list is empty");
            return;
        }
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strClause;
            bool bFirst = true;
            for (auto it = idlist.begin(); it != idlist.end(); ++it)
            {
                if (bFirst)
                {
                    bFirst = false;
                    strClause += itostr(*it);
                }
                else
                {
                    strClause += ("," + itostr(*it));
                }
            }
            String strSql = "select * from IMOrg where id in ( " + strClause + " )";
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    MBCAF::Proto::OrgInfo info;
                    uint32_t id;
                    uint32_t pid;
                    String orgname;
                    uint32_t state;
                    uint32_t priority;
                    resSet->getValue("id", id);
                    resSet->getValue("pid", pid);
                    resSet->getValue("orgName", orgname);
                    resSet->getValue("state", state);
                    resSet->getValue("priority", priority);
                    if (MBCAF::Proto::OrganizationStatusType_IsValid(state))
                    {
                        info.set_dept_id(id);
                        info.set_parent_dept_id(pid);
                        info.set_dept_name(orgname);
                        info.set_dept_status(MBCAF::Proto::OrganizationStateType(state));
                        info.set_priority(priority);
                        infolist.push_back(info);
                    }
                }
                delete  resSet;
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //----------------------------------------------------------------
    void Model_Organization::getOrg(uint32_t id, MBCAF::Proto::OrgInfo & info)
    {
        DatabaseManager * dbMag = M_Only(DatabaseManager);
        DatabaseConnect * dbConn = dbMag->getTempConnect("gsgsslave");
        if (dbConn)
        {
            String strSql = "select * from IMOrg where id = " + itostr(id);
            DatabaseResult * resSet = dbConn->execQuery(strSql.c_str());
            if (resSet)
            {
                while (resSet->nextRow())
                {
                    uint32_t id;
                    uint32_t pid;
                    String orgname;
                    uint32_t state;
                    uint32_t priority;
                    resSet->getValue("id", id);
                    resSet->getValue("pid", pid);
                    resSet->getValue("orgName", orgname);
                    resSet->getValue("state", state);
                    resSet->getValue("priority", priority);
                    if (MBCAF::Proto::OrganizationStatusType_IsValid(state))
                    {
                        info.set_dept_id(id);
                        info.set_parent_dept_id(pid);
                        info.set_dept_name(orgname);
                        info.set_dept_status(MBCAF::Proto::OrganizationStateType(state));
                        info.set_priority(priority);
                    }
                }
                delete resSet;
            }
            dbMag->freeTempConnect(dbConn);
        }
        else
        {
            Mlog("no db connection for gsgsslave");
        }
    }
    //----------------------------------------------------------------
}