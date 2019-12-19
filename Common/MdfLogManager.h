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

#ifndef _MDF_LOGManager_H_
#define _MDF_LOGManager_H_

#include "MdfPreInclude.h"

#define Mlog(fmt)				ACE_DEBUG((LM_INFO, fmt))
#define MlogDebug(fmt)			ACE_DEBUG((LM_DEBUG, fmt))
#define MlogWarning(fmt)		ACE_DEBUG((LM_WARNING, fmt))
#define MlogError(fmt)			ACE_ERROR((LM_ERROR, fmt))
#define MlogTrace(fmt)			ACE_DEBUG((LM_TRACE, fmt))
#define logError(fmt)			(LM_ERROR, fmt)
#define MlogErrorReturn			ACE_ERROR_RETURN

namespace Mdf
{
	/**
	@version 0.9.1
	*/
	class MdfNetAPI LogManager
	{
	public:
		LogManager();
		~LogManager();

		/**
		@version 0.9.1
		*/
		void enableError();

		/**
		@version 0.9.1
		*/
		void disableError();

		/**
		@version 0.9.1
		*/
		void enableDebug();

		/**
		@version 0.9.1
		*/
		void disableDebug();

		/**
		@version 0.9.1
		*/
		void enableInfo();

		/**
		@version 0.9.1
		*/
		void disableInfo();

		/**
		@version 0.9.1
		*/
		void enableWarning();

		/**
		@version 0.9.1
		*/
		void disableWarning();

		/**
		@version 0.9.1
		*/
		void redirectToDaemon(const ACE_TCHAR * prog_name = ACE_TEXT(""));

		/**
		@version 0.9.1
		*/
		void redirectToSyslog(const ACE_TCHAR * prog_name = ACE_TEXT(""));

		/**
		@param[in] output cout/cerr/
		@version 0.9.1
		*/
		void redirectToOstream(ACE_OSTREAM_TYPE * output);

		/**
		@version 0.9.1
		*/
		void redirectToFile(const char * filename);

		/**
		@version 0.9.1
		*/
		ACE_Log_Msg_Callback * redirectTo(ACE_Log_Msg_Callback * cb);

		/**
		@version 0.9.1
		*/
		void redirectToStderr();
	private:
		ofstream * mFileStream;
		ACE_OSTREAM_TYPE * mOutStream;
	};

	M_SingletonDef(LogManager);
}
#endif