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

#ifndef __MDF_EXCEPTION_H__
#define __MDF_EXCEPTION_H__

#include "MdfPreInclude.h"
#include "NetCore.h"

#ifdef MDF_Win32Client
#include "MdfWin32StrUtil.h"
#endif

#if defined(_MSC_VER)
#pragma warning(disable:4275)
#endif
#include <exception>

namespace Mdf
{
	/** 异常基类
	@version NIIEngine 3.0.0
	*/
	class MdfNetAPI Exception : public std::exception
	{
	public:
		Exception(const String & desc, const String & src);
		Exception(const String & desc, const String & src, Mlong mt, Mlong mi, 
			const String::value_type * t, const String::value_type * f, Mlong l);
		Exception(const Exception & o);

		/// 需要兼容std::exception
		virtual ~Exception() throw();

		/// 赋值运算符
		void operator = (const Exception & o);

		/** 获取消息类型
		@version NIIEngine 3.0.0
		*/
		Mlong getMessageType();

		/** 获取消息ID
		@version NIIEngine 3.0.0
		*/
		Mlong getMessageID();

		/** 获取发生行号
		@version NIIEngine 3.0.0
		*/
		Mlong getLine() const;

		/** 获取发生文件
		@version NIIEngine 3.0.0
		*/
		const String & getFile() const;

		/** 获取发生函数
		@version NIIEngine 3.0.0
		*/
		const String & getSource() const;

		/** 获取错误描述
		@version NIIEngine 3.0.0
		*/
		virtual const String & getVerbose() const;

		/** 返回描述这个错误的一个完整字符串
		@version NIIEngine 3.0.0
		*/
		const String & getErrorVerbose() const;

		/** 重写 std::exception::what
		@version NIIEngien 3.0.0
		*/
		char const * what() const throw();
	protected:
		Mlong mMessageType;
		Mlong mMessageID;
		Mlong mLine;
		String mType;
		String mDescrip;
		String mSrc;
		String mFile;
		mutable String mFullDescrip;
	};

#ifndef M_EXCEPT_DEF
#define M_EXCEPT_DEF(name) \
    class MdfNetAPI name##Exception : public Exception \
    {		\
    public :\
        name##Exception(const String & desc) :				\
			Exception(desc, _T(#name##"Exception")) {}			\
        name##Exception(const String & desc, Mlong mt, Mlong mi, const String::value_type * t, const String::value_type * file,	\
            Mlong line) : Exception(desc, _T(#name##"Exception"), mt, mi, t, file, line) {}				\
    };
#endif

#ifndef M_EXCEPT
#define M_EXCEPT(name, desc) throw name##Exception(desc, __FUNCTION__, __FILE__, __LINE__)
#endif

#ifndef M_EXCEPT2
#define M_EXCEPT2(name, desc, mt, mi) throw name##Exception(desc, mt, mi, __FUNCTION__, __FILE__, __LINE__)
#endif

	M_EXCEPT_DEF(IO);
	M_EXCEPT_DEF(Runtime);
	M_EXCEPT_DEF(Proto);
	M_EXCEPT_DEF(MessageType);
	M_EXCEPT_DEF(MessageID);
}
#if defined(_MSC_VER)
#pragma warning(default:4275)
#endif

#endif

