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

#include "MdfPgsqlDatabase.h"

namespace Mdf
{
    Mui32 PgsqlPrepareExec::mAutoName = 0;
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // PgsqlResult
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    PgsqlResult::PgsqlResult(PGresult * res)
    {
        mResult = res;
        mRow = 0;

        int attcnt = PQnfields(mResult);
        for(int i = 0; i < attcnt; ++i)
        {
            mFieldList.insert(make_pair(PQfname(mResult, i), i));
        }
    }
    //----------------------------------------------------------------
    PgsqlResult::~PgsqlResult()
    {
        if (mResult)
        {
            PQclear(mResult);
            mResult = 0;
        }
    }
    //----------------------------------------------------------------
    bool PgsqlResult::nextRow()
    {
        ++mRow;
        if (mRow < PQntuples(mResult))
            return true;
        return false;
    }
    //----------------------------------------------------------------
    void PgsqlResult::getValue(const String & key, Mi32 & out)
    {
        int idx = getIndex(key);
        if (idx != -1)
        {
            out = atoi(PQgetvalue(mResult, mRow, idx));
        }
        out = 0;
    }
    //----------------------------------------------------------------
    void PgsqlResult::getValue(const String & key, String & out)
    {
        int idx = getIndex(key);
        if (idx != -1)
        {
            out = PQgetvalue(mResult, mRow, idx);
        }
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // PgsqlPrepareExec
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    PgsqlPrepareExec::PgsqlPrepareExec() :
        mConnect(0),
        mParamData(0),
        mParamSize(0),
        mParamFormat(0)
    {
    }
    //----------------------------------------------------------------
    PgsqlPrepareExec::~PgsqlPrepareExec()
    {
        if (mParamData)
        {
            for (Mui32 i = 0; i < mParamCount; ++i)
            {
                free(mParamData[i]);
                mParamData[i] = 0;
            }
            free(mParamData);
            mParamData = 0;
        }
        if (mParamSize)
        {
            free(mParamSize);
            mParamSize = 0;
        }
        if (mParamFormat)
        {
            free(mParamFormat);
            mParamFormat = 0;
        }
    }
    //----------------------------------------------------------------
    Mui32 PgsqlPrepareExec::getInsertId(const String & tblname, const String & colname)
    {
        const char * paramv[2];
        paramv[0] = tblname.c_str();
        paramv[1] = colname.c_str();
        PGresult * res = PQexecParam(mConnect, "SELECT CURRVAL(pg_get_serial_sequence('$1', '$2'))",
            2, NULL, paramv, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(mConnect));
        }
        Mui32 temp = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return temp;
    }
    //----------------------------------------------------------------
    bool PgsqlPrepareExec::exec()
    {
        PGresult * res = PQexecPrepared(mConnect, mStmt.c_str(), mParamCount,
            mParamData, mParamSize, mParamFormat, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "SELECT failed: %s", PQerrorMessage(mConnect));
            PQclear(res);
            return false;
        }

        PQclear(res);
        return true;
    }
    //----------------------------------------------------------------
    bool PgsqlPrepareExec::prepare(DatabaseConnect * conn, const String & sql, Mui32 paramcnt)
    {
        PGconn * connect = static_cast<PgsqlDatabaseConnect>(conn)->getConnect();
        if (PQstatus(connect) != CONNECTION_OK)
        {
            return false;
        }
        char buf[256];
        sprintf(buf, "Mdf%s", ++mAutoName);

        mConnect = connect;
        mParamCount = paramcnt;
        mStmt = buf;

        PGresult * res = PQprepare(mConnect, mStmt.c_str(), sql.c_str(), mParamCount, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "SELECT failed: %s", PQerrorMessage(mConnect));
            PQclear(res);
            return false;
        }
        PQclear(res);
        if (mParamCount > 0)
        {
            mParamData = (char **)malloc(sizeof(char *) * mParamCount);
            mParamSize = (int *)malloc(sizeof(int) * mParamCount);
            mParamFormat = (int *)malloc(sizeof(int) * mParamCount);
            memset(mParamData, 0, sizeof(char *) * mParamCount);
            memset(mParamSize, 0, sizeof(int) * mParamCount);
            memset(mParamFormat, 0, sizeof(int) * mParamCount);
        }

        return true;
    }
    //----------------------------------------------------------------
    void PgsqlPrepareExec::setParam(Mui32 index, Mi32 & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamData[index] = (char *)malloc(sizeof(Mi32));
        *(Mi32 *)mParamData[index] = value;
        mParamSize[index] = sizeof(Mui32);
        mParamFormat[index] = 1;
    }
    //----------------------------------------------------------------
    void PgsqlPrepareExec::setParam(Mui32 index, Mui32 & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamData[index] = (char *)malloc(sizeof(Mui32));
        *(Mui32 *)mParamData[index] = value;
        mParamSize[index] = sizeof(Mui32);
        mParamFormat[index] = 1;
    }
    //----------------------------------------------------------------
    void PgsqlPrepareExec::setParam(Mui32 index, const String & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamData[index] = (char *)malloc(value.size() + 1);
        memcpy(mParamData[index], value.c_str(), value.size());
        mParamData[index][value.size()] = '\0';
        mParamSize[index] = value.size();
        mParamFormat[index] = 0;
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // PgsqlDatabaseConnect
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    PgsqlDatabaseConnect::PgsqlDatabaseConnect(DatabaseInstance * ins):
        DatabaseConnect(ins)
    {
        mConnect = NULL;
    }
    //----------------------------------------------------------------
    PgsqlDatabaseConnect::~PgsqlDatabaseConnect()
    {
        if (mConnect)
        {
            PQfinish(mConnect);
            mConnect = 0;
        }
    }
    //----------------------------------------------------------------
    int PgsqlDatabaseConnect::connect()
    {
        char buf[1024];
        sprintf(buf, "dbname=%s hostaddr=%s port=%d user=%s password=%s",
            mInstance->getDbName().c_str(), mInstance->getIP().c_str(), mInstance->getPort(),
            mInstance->getUserName().c_str(), mInstance->getPasswrod().c_str());

        mConnect = PQconnectdb(buf);
        if (PQstatus(mConnect) != CONNECTION_OK)
        {
            fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(mConnect));
            PQfinish(mConnect);
            mConnect = 0;
            return 2;
        }
        PQsetClientEncoding(mConnect, "utf8");

        return 0;
    }
    //----------------------------------------------------------------
    DatabaseResult * PgsqlDatabaseConnect::execQuery(const String & query)
    {
        if (PQstatus(mConnect) != CONNECTION_OK)
        {
            return 0;
        }

        PGresult * res = PQexec(mConnect, query.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(mConnect));
            PQclear(res);
            return 0;
        }
        return re;
    }
    //----------------------------------------------------------------
    bool PgsqlDatabaseConnect::exec(const String & query)
    {
        if (PQstatus(mConnect) != CONNECTION_OK)
        {
            return false;
        }

        PGresult * res = PQexec(mConnect, query.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(mConnect));
            PQclear(res);
            return false;
        }
        return true;
    }
    //----------------------------------------------------------------
    void PgsqlDatabaseConnect::escape(const String & content, String & out)
    {
        int error;
        char * temp = (char *)malloc(sizeof(char *) * (content.size() * 2 + 1));
        PQescapeStringConn(mConnect, temp, content.c_str(), content.size(), &error);
        out.assign(temp);
        free(temp);
        // error == 0 is ok
    }
    //----------------------------------------------------------------
    Mui32 PgsqlDatabaseConnect::getInsertId(const String & tblname, const String & colname)
    {
        const char * paramv[2];
        paramv[0] = tblname.c_str();
        paramv[1] = colname.c_str();
        PGresult * res = PQexecParam(mConnect, "SELECT CURRVAL(pg_get_serial_sequence('$1', '$2'))",
            2, NULL, paramv, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            fprintf(stderr, "FETCH ALL failed: %s", PQerrorMessage(mConnect));
        }
        Mui32 temp = atoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return temp;
    }
    //----------------------------------------------------------------
    PrepareExec * PgsqlDatabaseConnect::createPrepare() const
    {
        return new PgsqlPrepareExec();
    }
    //----------------------------------------------------------------
}