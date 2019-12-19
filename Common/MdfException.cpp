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

#include "MdfException.h"

#ifdef __BORLANDC__
#include <stdio.h>
#endif

#if defined(_MSC_VER)
#include <windows.h>
#include <dbghelp.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__HAIKU__)
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <cstdlib>
#endif

#ifndef _I18n
    #define _I18n(x) _T(x)
#endif

namespace Mdf
{
	//------------------------------------------------------------------------
	static void dumpBacktrace(MCount frames)
	{
#if defined(_DEBUG) || defined(DEBUG)
#if defined(_MSC_VER)
		SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES);

		if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
			return;

		HANDLE thread = GetCurrentThread();

		CONTEXT context;
		RtlCaptureContext(&context);

		STACKFRAME64 stackframe;
		ZeroMemory(&stackframe, sizeof(stackframe));
		stackframe.AddrPC.Mode = AddrModeFlat;
		stackframe.AddrStack.Mode = AddrModeFlat;
		stackframe.AddrFrame.Mode = AddrModeFlat;

#if _M_IX86
		stackframe.AddrPC.Offset = context.Eip;
		stackframe.AddrStack.Offset = context.Esp;
		stackframe.AddrFrame.Offset = context.Ebp;
		DWORD machine_arch = IMAGE_FILE_MACHINE_I386;
#elif _M_X64
		stackframe.AddrPC.Offset = context.Rip;
		stackframe.AddrStack.Offset = context.Rsp;
		stackframe.AddrFrame.Offset = context.Rbp;
		DWORD machine_arch = IMAGE_FILE_MACHINE_AMD64;
#endif

		char symbol_buffer[1024];
		ZeroMemory(symbol_buffer, sizeof(symbol_buffer));
		PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(symbol_buffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = sizeof(symbol_buffer) - sizeof(SYMBOL_INFO);

		//LogManager * logger = LogManager::getOnlyPtr();
		//logger->log("========== Start of Backtrace ==========");

		MCount frame_no = 0;
		while (StackWalk64(machine_arch, GetCurrentProcess(), thread, &stackframe,
			&context, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0) &&
			stackframe.AddrPC.Offset)
		{
			symbol->Address = stackframe.AddrPC.Offset;
			DWORD64 displacement = 0;
			char signature[256];

			if (SymFromAddr(GetCurrentProcess(), symbol->Address, &displacement, symbol))
				UnDecorateSymbolName(symbol->Name, signature, sizeof(signature), UNDNAME_COMPLETE);
			else
				sprintf_s(signature, sizeof(signature), "%llx", symbol->Address);

			IMAGEHLP_MODULE64 modinfo;
			modinfo.SizeOfStruct = sizeof(modinfo);

			const bool have_image_name =
				SymGetModuleInfo64(GetCurrentProcess(), symbol->Address, &modinfo);

			char outstr[512];
			sprintf_s(outstr, sizeof(outstr), "#%d %s +%#llx (%s)",
				frame_no, signature, displacement,
				(have_image_name ? modinfo.LoadedImageName : "????"));

			//logger->log(outstr, LML_CRITICAL);

			if (++frame_no >= frames)
				break;

			if (!stackframe.AddrReturn.Offset)
				break;
		}

		//logger->log("==========  End of Backtrace  ==========");

		SymCleanup(GetCurrentProcess());
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__HAIKU__)
		void * buffer[frames];
		const int received = backtrace(&buffer[0], frames);

		//LogManager * logger = LogManager::getOnlyPtr();

		//logger->logEvent("========== Start of Backtrace ==========");

		for (int i = 0; i < received; ++i)
		{
			char outstr[512];
			Dl_info info;
			if (dladdr(buffer[i], &info))
			{
				if (!info.dli_sname)
					snprintf(outstr, 512, "#%d %p (%s)", i, buffer[i], info.dli_fname);
				else
				{
					ptrdiff_t offset = static_cast<char*>(buffer[i]) -
						static_cast<char*>(info.dli_saddr);

					int demangle_result = 0;
					char * demangle_name = abi::__cxa_demangle(info.dli_sname, 0, 0, &demangle_result);
					snprintf(outstr, 512, "#%d %s +%#tx (%s)",
						i, demangle_name ? demangle_name : info.dli_sname, offset, info.dli_fname);
					std::free(demangle_name);
				}
			}
			else
				snprintf(outstr, 512, "#%d --- error ---", i);

			//logger->logEvent(outstr);
		}

		//logger->logEvent("==========  End of Backtrace  ==========");
#endif
#endif
	}
	//-----------------------------------------------------------------------
	Exception::Exception(const String & desc, const String & src) :
		mLine(0),
		mMessageType(0),
		mMessageID(0),
		mDescrip(desc),
		mSrc(src)
	{
	}
	//-----------------------------------------------------------------------
	Exception::Exception(const String & desc, const String & src, Mlong mt, Mlong mi,
		const String::value_type * type, const String::value_type * file, Mlong line) :
		mType(type),
		mMessageType(mt),
		mMessageID(mi),
		mDescrip(desc),
		mSrc(src),
		mFile(file),
		mLine(line)
	{
		/*
		// 记录此错误，从调试中标记，因为它可以被捕获和忽略
		if (LogManager::getOnlyPtr())
		{
			N_Only(Log).log(this->getErrorVerbose(), LML_CRITICAL, true);
		} */
	}
	//------------------------------------------------------------------------
	Exception::Exception(const Exception & o) :
		mLine(o.mLine),
		mType(o.mType),
		mMessageType(o.mMessageType),
		mMessageID(o.mMessageID),
		mDescrip(o.mDescrip),
		mSrc(o.mSrc),
		mFile(o.mFile)
	{
	}
	//-------------------------------------------------------------------------
	void Exception::operator = (const Exception & o)
	{
		mDescrip = o.mDescrip;
		mSrc = o.mSrc;
		mFile = o.mFile;
		mLine = o.mLine;
		mType = o.mType;
	}
	//-------------------------------------------------------------------------
	Exception::~Exception() throw()
	{
	}
	//-------------------------------------------------------------------------
	const String & Exception::getSource() const
	{
		return mSrc;
	}
	//-------------------------------------------------------------------------
	const String & Exception::getFile() const
	{
		return mFile;
	}
	//-------------------------------------------------------------------------
	Mlong Exception::getMessageType() 
	{ 
		return mMessageType; 
	}
	//-------------------------------------------------------------------------
	Mlong Exception::getMessageID() 
	{ 
		return mMessageID; 
	}
	//-------------------------------------------------------------------------
	Mlong Exception::getLine() const
	{
		return mLine;
	}
	//-------------------------------------------------------------------------
	const String & Exception::getVerbose() const
	{
		return mDescrip;
	}
	//-------------------------------------------------------------------------
	const String & Exception::getErrorVerbose() const
	{
		if (mFullDescrip.empty())
		{
			StringStream desc;
			desc << _I18n("NII (EXCEPTION: ") << mType << "): " << mDescrip << _I18n(" in ") << mSrc;

			if (mLine > 0)
			{
				desc << _I18n(" at: ") << mFile << _I18n("(line ") << mLine << ")";
			}

			mFullDescrip = desc.str();
		}
		return mFullDescrip;
	}
	//-------------------------------------------------------------------------
	char const * Exception::what() const throw()
	{
		if (mFullDescrip.empty())
		{
			StringStream desc;

			desc << _I18n("NII EXCEPTION: ") << mType << "): " << mDescrip << _I18n(" in ") << mSrc;

			if (mLine > 0)
			{
				desc << _I18n(" at: ") << mFile << _I18n("(line: ") << mLine << ")";
			}

			mFullDescrip = desc.str();
		}
		return (char *)mFullDescrip.c_str();
	}
	//-------------------------------------------------------------------------
}

