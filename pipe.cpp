//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  pipe.cpp
//
//   Description:  管道实现
//
//       Version:  1.0
//       Created:  2015年01月29日 15时40分34秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"pipe.hpp"

#include<cstdio>
#include<cstring>

namespace core
{
#ifdef WIN32

	const Pipe::Handle  Pipe::eInvalid = INVALID_HANDLE_VALUE;

    Pipe::Pipe()
    {
		char name[MAX_PATH];
		const size_t nsize = 4096;
		static unsigned long counter = 0;

		std::sprintf(name,
			"\\\\.\\pipe\\ezshCore.%08x.%08x",
			GetCurrentProcessId(),
			counter++
			);

		SECURITY_ATTRIBUTES att;
		std::memset(&att, 0, sizeof(att));
		att.nLength = sizeof(att);
		att.bInheritHandle = true;

		read_ = ::CreateNamedPipeA(
			name,
			PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE | PIPE_WAIT,
			1,             // Number of pipes
			nsize,         // Out buffer size
			nsize,         // In buffer size
			120 * 1000,    // Timeout in ms
			&att
			);

		write_ = ::CreateFileA(
			name,
			GENERIC_WRITE,
			0,                         // No sharing
			&att,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL                       // Template file
			);
    }

#else
    Pipe::Pipe()
    {
        Handle fds[2]={eInvalid, eInvalid};
        ::pipe(fds);
        read_=fds[0];
        write_=fds[1];
    }

#endif


}

