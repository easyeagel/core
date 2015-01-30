//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  pipe.hpp
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

#pragma once

#ifdef WIN32
    #include<windows.h>
#else
    #include<unistd.h>
#endif


namespace core
{

class Pipe
{
public:
    Pipe();

#ifdef WIN32
    typedef HANDLE Handle;
	static const Handle eInvalid;

    void readClose()
    {
        if(read_==eInvalid)
            return;
        ::CloseHandle(read_);
        read_=eInvalid;
    }

    void writeClose()
    {
        if(write_==eInvalid)
            return;
        ::CloseHandle(write_);
        write_=eInvalid;
    }

#else
    typedef int Handle;
	static const Handle eInvalid = -1;

    void readClose()
    {
        if(read_==eInvalid)
            return;
        ::close(read_);
        read_=eInvalid;
    }

    void writeClose()
    {
        if(write_==eInvalid)
            return;
        ::close(write_);
        write_=eInvalid;
    }

#endif

    void close()
    {
        readClose();
        writeClose();
    }

	Handle readGet() const
	{
		return read_;
	}

	Handle writeGet() const
	{
		return write_;
	}

	Handle readReleaseGet()
	{
		const auto t = read_;
		read_ = eInvalid;
		return t;
	}

	Handle writeReleaseGet()
	{
		const auto t = write_;
		write_ = eInvalid;
		return t;
	}

	bool good() const
	{
		return read_ != eInvalid && write_ != eInvalid;
	}

    ~Pipe()
    {
        close();
    }

private:
    Handle read_=eInvalid;
	Handle write_ = eInvalid;
};

}


