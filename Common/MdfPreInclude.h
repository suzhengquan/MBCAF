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

#ifndef __MDF_PRE_INCLUDE_H__
#define __MDF_PRE_INCLUDE_H__

#include "MdfConfig.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <WinBase.h>
#include <Windows.h>
#include <direct.h>
#include <stdint.h>
#else
#ifdef __APPLE__
#include <sys/event.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>	// syscall(SYS_gettid)
#else
#include <sys/epoll.h>
#endif
#include <pthread.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>		// define int8_t ...
#include <signal.h>
#include <unistd.h>
#define closesocket close
#define ioctlsocket ioctl
#endif

#ifdef __GNUC__
#include <ext/hash_map>
using namespace __gnu_cxx;
namespace __gnu_cxx 
{
	template<> struct hash<std::string> 
	{
		size_t operator()(const std::string& x) const 
		{
			return hash<const char*>()(x.c_str());
		}
	};
}
#else
#include <hash_map>
using namespace stdext;
#endif

#include <set>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdexcept>
#include <tchar.h>

#if _UNICODE
typedef std::wstring		String;
typedef std::vector<String> StringList;
typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> > StringStream;
#define ACE_USES_WCHAR
#else
typedef std::string			String;
typedef std::vector<String> StringList;
typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char> > StringStream;
#endif

using namespace std;

#ifdef _WIN32
typedef int socklen_t;
#else
typedef int	SOCKET;
typedef int bool;
#ifndef  __APPLE__
	const int TRUE = 1;
	const int FALSE = 0;
#endif
const int SOCKET_ERROR = -1;
const int INVALID_SOCKET = -1;
#endif

#define M_AtomicAdd(src_ptr, v)				(void)__sync_add_and_fetch(src_ptr, v)
#define M_AtomicAddFetch(src_ptr, v)		__sync_add_and_fetch(src_ptr, v)
#define M_AtomicSubFetch(src_ptr, v)		__sync_sub_and_fetch(src_ptr, v)
#define M_AtomicFetch(src_ptr)				__sync_add_and_fetch(src_ptr, 0)
#define M_AtomicSet(src_ptr, v)				(void)__sync_bool_compare_and_swap(src_ptr, *(src_ptr), v)

typedef  volatile long atomic_t;

typedef int net_handle_t;
typedef int conn_handle_t;

#include <ace/config-lite.h>
#include <ace/Time_Value.h>
#include <ace/Basic_Types.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Event_Handler.h>
#include <ace/streams.h>
#include <ace/Log_Msg.h>
#include <ace/Log_Msg_Callback.h>
#include <ace/INET_Addr.h>
#include <ace/Reactor.h>
#include <ace/Synch.h>
#include <ace/Singleton.h>
#include <ace/streams.h>
#include <ace/Time_Value.h>
#include <ace/Auto_Ptr.h>
#include <ace/Signal.h>
#include <ace/Thread_Manager.h>
#include <ace/TP_Reactor.h>
#include <ace/Synch_Traits.h>
#include <ace/Null_Condition.h>
#include <ace/Null_Mutex.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Connector.h>
#include <ace/Connector.h>
#include <ace/Svc_Handler.h>
#include <ace/Reactor_Notification_Strategy.h>
#include <ace/OS_NS_stdio.h>
#include <ace/OS_NS_errno.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/OS.h>
#include <ace/Get_Opt.h>
#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Service_Object.h>
#include <ace/Task.h>
#include <ace/Init_ACE.h>
#include <ace/Condition_T.h>
#include <ace/Dirent.h>
#include <ace/File_Lock.h>
#include <ace/SOCK_SEQPACK_Association.h>

namespace google{
namespace protobuf{
	class MessageLite;
}
}

#ifdef MDF_BUILD_DLL
#define MdfNetAPI ACE_Proper_Export_Flag
#else
#define MdfNetAPI ACE_Proper_Import_Flag
#endif

#include "ProtocolBuffer/MBCAF.Proto.pb.h"

#define M_SingletonDef(mag) typedef ACE_Singleton<mag, ACE_Null_Mutex> mag##Singleton
#define M_Only(mag) mag##Singleton::instance()
#define M_OnlyClose(mag) mag##Singleton::close()
#define M_SingletonImpl(mag) ACE_SINGLETON_TEMPLATE_INSTANTIATE(ACE_Singleton, mag, ACE_Null_Mutex)

typedef bool			Mbool;
typedef ACE_INT8		Mi8;
typedef wchar_t			Mw16;
typedef ACE_UINT8		Mui8;
typedef ACE_INT16		Mi16;
typedef ACE_UINT16		Mui16;
typedef ACE_INT32		Mi32;
typedef ACE_UINT32		Mui32;
typedef ACE_INT64		Mi64;
typedef ACE_UINT64		Mui64;
typedef ACE_UINT32		MCount;
typedef long            Mlong;

namespace Mdf
{
	#ifdef _WIN32
	#define	snprintf sprintf_s
	#endif
	class SocketClientPrc;
	class SocketAcceptPrc;
	class SocketServerPrc;
    class SSLClientPrc;
    class SSLAcceptPrc;
    class SSLServerPrc;
	class ServerIO;
	class ClientIO;
	class MemStream;
	class ThreadMain;
	class Thread;

    typedef ACE_HANDLE		ConnectID;
    typedef ACE_UINT64		TimeDurMS;
    typedef ACE_UINT64		TimeDurUS;
	typedef char            utf8;

	typedef ACE_Atomic_Op<ACE_Thread_Mutex, bool> ST_Mbool;

    static const int BASE = 0x100000;

    #define M_Heartbeat_Interval			10000
    #define M_Server_Timeout				20000
    #define M_Client_Heartbeat_Timeout		30000
    #define M_Client_Timeout				90000
	#define M_HTTP_Timeout				    80000
    #define M_MobileClient_Timeout			200000
    #define M_SocketBlockSize				2048
	#define M_SocketOutSize					(64 * 1024)
	#define M_ReConnect_Max					20
	#define M_ReConnect_Min					6
	#define M_ReConnect_Interval			50000

	#define M_Trace(info) ACE_TRACE(info)

	enum
	{
		ServerConnectInc = 1,
		USER_CNT_DEC = 2,
	};

	enum
	{
		IM_GROUP_SETTING_PUSH = 1,
	};

	enum
	{
		IM_PUSH_TYPE_NORMAL = 1,
		IM_PUSH_TYPE_SILENT = 2,
	};

	enum
	{
		PL_StateOn = 1,
		PL_StateOff = 0,
	};

	enum
	{
		GENDER_UNKNOWN = 0,
		GENDER_MAN = 1,
		GENDER_WOMAN = 2,
	};

	#define CLIENT_TYPE_FLAG_NONE    0x00
	#define CLIENT_TYPE_FLAG_PC      0x01
	#define CLIENT_TYPE_FLAG_MOBILE  0x02
	#define CLIENT_TYPE_FLAG_BOTH    0x03

	#define M_PCLoginCheck(type) ((type & 0x10) == 0x00)

	#define M_MobileLoginCheck(type) ((type & 0x10) == 0x10)

	#define M_GroupMessageCheck(type) ((MBCAF::Proto::MT_GroupText == type) || (MBCAF::Proto::MT_GroupAudio == type))

    #define M_SingleMessageCheck(type) ((MBCAF::Proto::MT_Text == type) || (MBCAF::Proto::MT_Audio == type))
}

#endif

