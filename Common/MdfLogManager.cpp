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

#include "MdfLogManager.h"

namespace Mdf
{
	M_SingletonImpl(LogManager);
	//-----------------------------------------------------------------------
	LogManager::LogManager() :
		mFileStream(0)
	{
		if (ACE_LOG_MSG->open(_T("Mdf"), ACE_Log_Msg::OSTREAM) == -1)
			return;

		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR | ACE_Log_Msg::LOGGER);

		mFileStream = new ofstream();
		const char * filename = "log.text";
		ofstream outfile(filename, ios::out | ios::app);
		if (outfile.bad())
		{
			mFileStream->close();
			delete mFileStream;
			mFileStream = 0;
			return;
		}
		redirectToOstream((ACE_OSTREAM_TYPE *)mFileStream);
	}
	//-----------------------------------------------------------------------
	LogManager::~LogManager()
	{
		if (mFileStream)
		{
			mFileStream->close();
			delete mFileStream;
			mFileStream = 0;
		}
	}
	//-----------------------------------------------------------------------
	void LogManager::enableError()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_SET_BITS(mask, LM_ERROR);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::disableError()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_CLR_BITS(mask, LM_ERROR);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::enableDebug()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_SET_BITS(mask, LM_DEBUG);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::disableDebug()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_CLR_BITS(mask, LM_DEBUG);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::enableInfo()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_SET_BITS(mask, LM_INFO);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::disableInfo()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_CLR_BITS(mask, LM_INFO);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::enableWarning()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_SET_BITS(mask, LM_WARNING);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::disableWarning()
	{
		u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
		ACE_CLR_BITS(mask, LM_WARNING);
		ACE_LOG_MSG->priority_mask(mask, ACE_Log_Msg::PROCESS);
	}
	//-----------------------------------------------------------------------
	void LogManager::redirectToSyslog(const ACE_TCHAR * prog_name)
	{
		ACE_LOG_MSG->open(prog_name, ACE_Log_Msg::SYSLOG, prog_name);
	}
	//-----------------------------------------------------------------------
	void LogManager::redirectToDaemon(const ACE_TCHAR * prog_name)
	{
		ACE_LOG_MSG->open(prog_name, ACE_Log_Msg::LOGGER, ACE_DEFAULT_LOGGER_KEY);
	}
	//-----------------------------------------------------------------------
	void LogManager::redirectToOstream(ACE_OSTREAM_TYPE * output)
	{
		mOutStream = output;
		ACE_LOG_MSG->msg_ostream(mOutStream);
		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR | ACE_Log_Msg::LOGGER);
		ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
	}
	//-----------------------------------------------------------------------
	void LogManager::redirectToFile(const char * filename)
	{
		mFileStream = new ofstream();
		mFileStream->open(filename, ios::out | ios::app);
		this->redirectToOstream((ACE_OSTREAM_TYPE *)mFileStream);
	}
	//-----------------------------------------------------------------------
	void LogManager::redirectToStderr()
	{
		ACE_LOG_MSG->clr_flags(ACE_Log_Msg::OSTREAM | ACE_Log_Msg::LOGGER);
		ACE_LOG_MSG->set_flags(ACE_Log_Msg::STDERR);
	}
	//-----------------------------------------------------------------------
	ACE_Log_Msg_Callback * LogManager::redirectTo(ACE_Log_Msg_Callback * cb)
	{
		ACE_Log_Msg_Callback * old = ACE_LOG_MSG->msg_callback(cb);
		if (cb == 0)
			ACE_LOG_MSG->clr_flags(ACE_Log_Msg::MSG_CALLBACK);
		else
			ACE_LOG_MSG->set_flags(ACE_Log_Msg::MSG_CALLBACK);

		return old;
	}
	//-----------------------------------------------------------------------
}