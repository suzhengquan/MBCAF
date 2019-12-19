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

namespace Mdf
{
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // MysqlResult
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    MysqlResult::MysqlResult(MYSQL_RES * res)
    {
        mResult = res;

        int attcnt = mysql_num_fields(mResult);
        MYSQL_FIELD * fields = mysql_fetch_fields(mResult);
        for (int i = 0; i < attcnt; ++i)
        {
            mFieldList.insert(make_pair(fields[i].name, i));
        }
    }
    //----------------------------------------------------------------
    MysqlResult::~MysqlResult()
    {
        if (mResult)
        {
            mysql_free_result(mResult);
            mResult = NULL;
        }
    }
    //----------------------------------------------------------------
    bool MysqlResult::nextRow()
    {
        mRow = mysql_fetch_row(mResult);
        if (mRow)
        {
            return true;
        }
        return false;
    }
    //----------------------------------------------------------------
    void MysqlResult::getValue(const String & key, Mi32 & out)
    {
        int idx = getIndex(key);
        if (idx == -1)
        {
            out = 0;
        }
        else
        {
            out = atoi(mRow[idx]);
        }
    }
    //----------------------------------------------------------------
    void MysqlResult::getValue(const String & key, String & out)
    {
        int idx = getIndex(key);
        if (idx != -1)
        {
            out = mRow[idx];
        }
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // MysqlPrepareExec
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    MysqlPrepareExec::MysqlPrepareExec() :
        mStmt(0),
        mParamBind(0)
    {
    }
    //----------------------------------------------------------------
    MysqlPrepareExec::~MysqlPrepareExec()
    {
        if (mStmt)
        {
            mysql_stmt_close(mStmt);
            mStmt = NULL;
        }

        if (mParamBind)
        {
            free(mParamBind);
            mParamBind = 0;
        }
    }
    //----------------------------------------------------------------
    bool MysqlPrepareExec::prepare(DatabaseConnect * conn, const String & sql, Mui32 paramcnt)
    {
        MYSQL * connect = static_cast<MysqlDatabaseConnect>(conn)->getConnect();
        mysql_ping(connect);

        mStmt = mysql_stmt_init(connect);
        if (!mStmt)
        {
            Mlog("mysql_stmt_init failed");
            return false;
        }

        if (mysql_stmt_prepare(mStmt, sql.c_str(), sql.size()))
        {
            Mlog("mysql_stmt_prepare failed: %s", mysql_stmt_error(mStmt));
            return false;
        }

        mParamCount = mysql_stmt_param_count(mStmt);
        if (mParamCount > 0)
        {
            assert(paramcnt == mParamCount);
            mParamBind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * mParamCount);
            memset(mParamBind, 0, sizeof(MYSQL_BIND) * mParamCount);
        }

        return true;
    }
    //----------------------------------------------------------------
    void MysqlPrepareExec::setParam(Mui32 index, Mi32 & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamBind[index].buffer_type = MYSQL_TYPE_LONG;
        mParamBind[index].buffer = &value;
    }
    //----------------------------------------------------------------
    void MysqlPrepareExec::setParam(Mui32 index, Mui32 & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamBind[index].buffer_type = MYSQL_TYPE_LONG;
        mParamBind[index].buffer = &value;
    }
    //----------------------------------------------------------------
    void MysqlPrepareExec::setParam(Mui32 index, const String & value)
    {
        if (index >= mParamCount)
        {
            Mlog("index too large: %d", index);
            return;
        }

        mParamBind[index].buffer_type = MYSQL_TYPE_STRING;
        mParamBind[index].buffer = (char*)value.c_str();
        mParamBind[index].buffer_length = value.size();
    }
    //----------------------------------------------------------------
    bool MysqlPrepareExec::exec()
    {
        if (!mStmt)
        {
            Mlog("no mStmt");
            return false;
        }

        if (mysql_stmt_bind_param(mStmt, mParamBind))
        {
            Mlog("mysql_stmt_bind_param failed: %s", mysql_stmt_error(mStmt));
            return false;
        }

        if (mysql_stmt_execute(mStmt))
        {
            Mlog("mysql_stmt_execute failed: %s", mysql_stmt_error(mStmt));
            return false;
        }

        if (mysql_stmt_affected_rows(mStmt) == 0)
        {
            Mlog("exec have no effect");
            return false;
        }

        return true;
    }

    //----------------------------------------------------------------
    Mui32 MysqlPrepareExec::getInsertId(const String & tblname, const String & colname)
    {
        return mysql_stmt_insert_id(mStmt);
    }
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    // MysqlDatabaseConnect
    //----------------------------------------------------------------
    //----------------------------------------------------------------
    MysqlDatabaseConnect::MysqlDatabaseConnect(DatabaseInstance * ins):
        DatabaseConnect(ins)
    {
        mConnect = 0;
    }
    //----------------------------------------------------------------
    MysqlDatabaseConnect::~MysqlDatabaseConnect()
    {
        if (mConnect)
        {
            mysql_close(mConnect);
            mConnect = 0;
        }
    }
    //----------------------------------------------------------------
    int MysqlDatabaseConnect::connect()
    {
        mConnect = mysql_init(NULL);
        if (!mConnect)
        {
            Mlog("mysql_init failed");
            return 1;
        }

        my_bool reconnect = true;
        mysql_options(mConnect, MYSQL_OPT_RECONNECT, &reconnect);
        mysql_options(mConnect, MYSQL_SET_CHARSET_NAME, "utf8mb4");

        if (!mysql_real_connect(mConnect, mInstance->getIP(), mInstance->getUserName(), mInstance->getPasswrod(),
            mInstance->getDbName(), mInstance->getPort(), NULL, 0))
        {
            Mlog("mysql_real_connect failed: %s", mysql_error(mConnect));
            mysql_close(mConnect);
            mConnect = 0;
            return 2;
        }

        return 0;
    }

    //----------------------------------------------------------------
    DatabaseResult * MysqlDatabaseConnect::execQuery(const String & query)
    {
        mysql_ping(mConnect);

        if (mysql_real_query(mConnect, query, strlen(query)))
        {
            Mlog("mysql_real_query failed: %s, sql: %s", mysql_error(mConnect), query);
            return 0;
        }

        MYSQL_RES * res = mysql_store_result(mConnect);
        if (!res)
        {
            Mlog("mysql_store_result failed: %s", mysql_error(mConnect));
            return 0;
        }

        MysqlResult * re = new MysqlResult(res);
        return re;
    }
    //----------------------------------------------------------------
    bool MysqlDatabaseConnect::exec(const String & query)
    {
        mysql_ping(mConnect);

        if (mysql_real_query(mConnect, query, strlen(query)))
        {
            Mlog("mysql_real_query failed: %s, sql: %s", mysql_error(mConnect), query);
            return false;
        }

        if (mysql_affected_rows(mConnect) > 0)
        {
            return true;
        }

        return false;
    }
    //----------------------------------------------------------------
    void MysqlDatabaseConnect::escape(const String & content, String & out)
    {
        char * temp = (char *)malloc(sizeof(char *) * (content.size() * 2 + 1));
        mysql_real_escape_string(mConnect, temp, content.c_str(), content.size());
        out.assign(temp);
        free(temp);
    }
    //----------------------------------------------------------------
    Mui32 MysqlDatabaseConnect::getInsertId()
    {
        return (Mui32)mysql_insert_id(mConnect);
    }
    //----------------------------------------------------------------
    PrepareExec * MysqlDatabaseConnect::createPrepare() const
    {
        return new MysqlPrepareExec();
    }
    //----------------------------------------------------------------
}