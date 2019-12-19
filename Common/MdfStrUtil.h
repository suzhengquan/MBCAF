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

#ifndef _MDF_StrUtil_H_
#define _MDF_StrUtil_H_

#include "MdfPreInclude.h"

namespace Mdf
{
	/**
	@version 0.9.1
	*/
	class MdfNetAPI StrUtil
	{
	public:
		/**
		@version 0.9.1
		*/
		static void split(const String & str, StringList & out, const String & delim = _T("\t\n "), bool trimEmpty = false);

		/**
		@version 0.9.1
		*/
		static void compact(const std::vector<String> & tokens, String & out);

		/**
		@version 0.9.1
		*/
		static String itostr(Mui32 value);

		/**
		@version 0.9.1
		*/
		static Mui32 strtoi(const String & value);

		/**
		@version 0.9.1
		*/
		static void idtourl(Mui32 in, char * out);

		/**
		@version 0.9.1
		*/
		static Mui32 urltoid(const String & url);

		/**
		@version 0.9.1
		*/
		static void replace(String & str, String & in, char makr, Mui32 & pos);

		/**
		@version 0.9.1
		*/
		static void replace(String & str, Mui32 in, char makr, Mui32 & pos);

		/**
		@version 0.9.1
		*/
		static String UrlEncode(const String & sIn);

		/**
		@version 0.9.1
		*/
		static String UrlDecode(const String & sIn);

		/**
		@version 0.9.1
		*/
		static const std::string ws2s(const std::wstring & src);

		/**
		@version 0.9.1
		*/
		static const std::wstring s2ws(const std::string & src);
	};
}
#endif
