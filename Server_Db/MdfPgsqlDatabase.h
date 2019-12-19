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

#include "MdfPreInclude.h"
#include "MdfDatabaseManager.h"
#include <libpq-fe.h>
#include <stdio.h>

namespace Mdf
{
    /**
    @version 0.9.1
    */
    class PgsqlResult : public DatabaseResult
    {
    public:
        PgsqlResult(PGresult * res);
        virtual ~PgsqlResult();

        /// @copydetails DatabaseResult::nextRow
        bool nextRow();

        /// @copydetails DatabaseResult::getValue
        void getValue(const String & key, Mi32 & out);

        /// @copydetails DatabaseResult::getValue
        void getValue(const String & key, String & out);
    protected:
        PgsqlResult() {}
    private:
        PGresult * mResult;
        Mui32 mRow;
        map<String, Mui32> mFieldList;
    };

    /**
    @version 0.9.1
    */
    class PgsqlPrepareExec : public PrepareExec
    {
    public:
        PgsqlPrepareExec();
        virtual ~PgsqlPrepareExec();

        /// @copydetails PrepareExec::exec
        bool exec();

        /// @copydetails PrepareExec::prepare
        bool prepare(DatabaseConnect * connect, const String & sql, Mui32 paramcnt);

        /// @copydetails PrepareExec::setParam
        void setParam(Mui32 index, Mi32 & value);

        /// @copydetails PrepareExec::setParam
        void setParam(Mui32 index, Mui32 & value);

        /// @copydetails PrepareExec::setParam
        void setParam(Mui32 index, const String & value);

        /// @copydetails PrepareExec::getInsertId
        Mui32 getInsertId(const String & tblname, const String & colname);
    private:
        String mStmt;
        PGconn * mConnect;
        char *const* mParamData;
        int * mParamSize;
        int * mParamFormat;
        Mui32 mParamCount;
        static Mui32 mAutoName;
    };

    /**
    @version 0.9.1
    */
    class PgsqlDatabaseConnect : public DatabaseConnect
    {
    public:
        PgsqlDatabaseConnect(DatabaseInstance * ins);
        virtual ~PgsqlDatabaseConnect();

        /// @copydetails DatabaseConnect::connect
        int connect();

        /// @copydetails DatabaseConnect::exec
        bool exec(const String & query);

        /// @copydetails DatabaseConnect::execQuery
        DatabaseResult * execQuery(const String & query);

        /// @copydetails DatabaseConnect::escape
        void escape(const String & content, String & out);

        /// @copydetails DatabaseConnect::beginTransaction
        bool beginTransaction()
        {
            PGresult * res = PQexec(mConnect, "\set AUTOCOMMIT off");
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(mConnect));
                PQclear(res);
                return false;
            }
            PQclear(res);

            res = PQexec(mConnect, "BEGIN TRANSACTION;");
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(mConnect));
                PQclear(res);
                return false;
            }
            PQclear(res);
            return true;
        }

        /// @copydetails DatabaseConnect::endTransaction
        bool endTransaction()
        {
            PGresult * res = PQexec(mConnect, "END TRANSACTION;");
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(mConnect));
                PQclear(res);
                return false;
            }
            PQclear(res);

            res = PQexec(mConnect, "\set AUTOCOMMIT on");
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(mConnect));
                PQclear(res);
                return false;
            }
            PQclear(res);
            return true;
        }

        /// @copydetails DatabaseConnect::rollbackTransaction
        bool rollbackTransaction()
        {
            PGresult * res = PQexec(mConnect, "ROLLBACK;");
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(mConnect));
                PQclear(res);
                return false;
            }
            PQclear(res);
            return true;
        }

        /// @copydetails DatabaseConnect::getInsertId
        Mui32 getInsertId(const String & tblname, const String & colname);

        /// @copydetails DatabaseConnect::createPrepare
        PrepareExec * createPrepare() const;

        /**
        @version 0.9.1
        */
        inline PGconn * getConnect() const { return mConnect; }
    private:
        PGconn * mConnect;
        DatabaseInstance * mInstance;
    };
}