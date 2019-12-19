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

#include "HttpParserWrapper.h"

#define Http_Referer_MaxSize	32

#define Http_Referer			0x000001
#define Http_Forward			0x000002
#define Http_UserAgent			0x000004
#define Http_ContentType		0x000008
#define Http_ContentLength		0x000010
#define Http_Host				0x000020

namespace Mdf
{
	//-----------------------------------------------------------------------
	int _mdf_http_url(http_parser * parser, const char * at, size_t length, void * obj)
	{
		static_cast<HttpParser *>(obj)->setUrl(at, length);
		return 0;
	}
	//-----------------------------------------------------------------------
	int _mdf_http_body(http_parser * parser, const char * at, size_t length, void * obj)
	{
		static_cast<HttpParser *>(obj)->setBodyContent(at, length);
		return 0;
	}
	//-----------------------------------------------------------------------
	int _mdf_http_headerfield(http_parser* parser, const char *at, size_t length, void* obj)
	{
		HttpParser * temp = static_cast<HttpParser *>(obj);
		if (!temp->isReadReferer())
		{
			if (strncasecmp(at, "Referer", 7) == 0)
			{
				temp->addMark(Http_Referer);
			}
		}
		else if (!temp->isReadForward())
		{
			if (strncasecmp(at, "X-Forwarded-For", 15) == 0)
			{
				temp->addMark(Http_Forward);
			}
		}
		else if (!temp->isReadUserAgent())
		{
			if (strncasecmp(at, "User-Agent", 10) == 0)
			{
				temp->addMark(Http_UserAgent);
			}
		}
		else if (!temp->isReadContentType())
		{
			if (strncasecmp(at, "Content-Type", 12) == 0)
			{
				temp->addMark(Http_ContentType);
			}
		}
		else if (!temp->HasReadContentLen())
		{
			if (strncasecmp(at, "Content-Length", 14) == 0)
			{
				temp->addMark(Http_ContentLength);
			}
		}
		else if (!temp->isReadHost())
		{
			if (strncasecmp(at, "Host", 4) == 0)
			{
				temp->addMark(Http_Host);
			}
		}
		return 0;
	}
	//-----------------------------------------------------------------------
	int _mdf_http_headervalue(http_parser * parser, const char * at, size_t length, void* obj)
	{
		HttpParser * temp = static_cast<HttpParser *>(obj);
		if (temp->getMark() & Http_Referer)
		{
			size_t referer_len = (length > Http_Referer_MaxSize) ? Http_Referer_MaxSize : length;
			temp->setReferer(at, referer_len);
			temp->removeMark(Http_Referer);
		}

		if (temp->getMark() & Http_Forward)
		{
			temp->setForward(at, length);
			temp->removeMark(Http_Forward);
		}

		if (temp->getMark() & Http_UserAgent)
		{
			temp->setUserAgent(at, length);
			temp->removeMark(Http_UserAgent);
		}

		if (temp->getMark() & Http_ContentType)
		{
			temp->setContentType(at, length);
			temp->removeMark(Http_ContentType);
		}

		if (temp->getMark() & Http_ContentLength)
		{
			String strContentLen(at, length);
			temp->setContentLength(atoi(strContentLen.c_str()));
			temp->removeMark(Http_ContentLength);
		}

		if (temp->getMark() & Http_Host)
		{
			temp->setHost(at, length);
			temp->removeMark(Http_Host);
		}
		return 0;
	}
	//-----------------------------------------------------------------------
	int _mdf_http_headerscomplete(http_parser * parser, void * obj)
	{
		static_cast<HttpParser *>(obj)->setTotalLength(parser->nread + (Mui32)parser->content_length);
		return 0;
	}
	//-----------------------------------------------------------------------
	int _mdf_http_messagecomplete(http_parser * parser, void * obj)
	{
		static_cast<HttpParser *>(obj)->complete();
		return 0;
	}
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	// HttpParser
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	void HttpParser::parse(const char * buf, MCount len):
		mMark(0)
	{
		http_parser_init(&mParser, HTTP_REQUEST);
		memset(&mParserParam, 0, sizeof(mParserParam));
		mParserParam.on_url = _mdf_http_url;
		mParserParam.on_header_field = _mdf_http_headerfield;
		mParserParam.on_header_value = _mdf_http_headervalue;
		mParserParam.on_headers_complete = _mdf_http_headerscomplete;
		mParserParam.on_body = _mdf_http_body;
		mParserParam.on_message_complete = _mdf_http_messagecomplete;
		mParserParam.object = this;

		mComplete = false;
		mTotalLength = 0;
		mContentLength = 0;

		http_parser_execute(&mParser, &mParserParam, buf, len);
	}
	//-----------------------------------------------------------------------
	void HttpParser::addMark(Mui32 m)
	{
		mMark |= m;
	}
	//-----------------------------------------------------------------------
	void HttpParser::removeMark(Mui32 m)
	{
		mMark &= ~m;
	}
	//-----------------------------------------------------------------------
	Mui32 HttpParser::getMark() const
	{
		return mMark;
	}
	//-----------------------------------------------------------------------
}