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

#ifndef _MDF_HttpParser_H_
#define _MDF_HttpParser_H_

#include "MdfPreInclude.h"
#include "http_parser.h"

namespace Mdf
{
	/**
	@version 0.9.1
	*/
	class HttpParser
	{
	public:
		HttpParser() {}
		~HttpParser() {}

		void parse(const char * buf, MCount len);

		inline bool isComplete() const { return mComplete; }

		inline void complete()  { mComplete = true; }

		inline void setUrl(const char * url, size_t len) { mURL.assign(url, len); }
		inline const String & getUrl() const { return mURL; }

		inline const String & getBodyContent() const { return mBodyContent; }
		inline Mui32 getBodyContentLength() const { return mBodyContent.length(); }

		inline void setReferer(const char * data, size_t len) { mReferer.assign(data, len); }
		inline const String & getReferer() const { return mReferer; }

		inline void setForward(const char * data, size_t len) { mForward.assign(data, len); }
		inline const String & getForward() const { return mForward; }

		inline void setUserAgent(const char * data, size_t len) { mUserAgent.assign(data, len); }
		inline const String & getUserAgent() const { return mUserAgent; }

		inline void setBodyContent(const char * data, size_t len) { mBodyContent.assign(data, len); }
		inline char getMethod() { return mParser.method; }

		inline void setContentType(const char * data, size_t len) { mContentType.assign(data, len); }
		inline const String & getContentType() const { return mContentType; }

		inline void setContentLength(Mui32 len) { mContentLength = len; }
		inline Mui32  getContentLength() const { return mContentLength; }

		inline void setHost(const char * host, size_t len) { mHost.append(host, len); }
		inline const String & getHost() const { return mHost; }
		
		inline void setTotalLength(Mui32 len) { mTotalLength = len; }
		inline Mui32 getTotalLength() const { return mTotalLength; }
		
		inline bool isReadReferer() const { return mReferer.size() > 0; }

		inline bool isReadForward() const { return mForward.size() > 0; }

		inline bool isReadUserAgent() const { return mUserAgent.size() > 0; }

		inline bool isReadContentType() const { return mContentType.size() > 0; }

		inline bool isReadContentLength() const { return mContentLength != 0; }

		inline bool isReadHost() const { return mHost.size() > 0; }

		void addMark(Mui32 m);

		void removeMark(Mui32 m);

		Mui32 getMark() const;
	private:
		String mURL;
		String mHost;
		String mBodyContent;
		String mReferer;
		String mForward;
		String mUserAgent;
		String mContentType;
		Mui32 mContentLength;
		Mui32 mTotalLength;
		http_parser mParser;
		http_parser_settings mParserParam;
		Nui32 mMark;
		bool mComplete;
	};
}
#endif
