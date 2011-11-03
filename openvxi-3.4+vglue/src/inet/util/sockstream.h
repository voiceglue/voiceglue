// sockstream.h -*- C++ -*- socket library

// Copyright (C) 1992,1993,1994 Gnanasekaran Swaminathan <gs4t@virginia.edu>
//
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided the above copyright notice and this
// paragraph are preserved on all copies.  This software is provided "as is"
// with no express or implied warranty.
//
// Version: 17Oct95 1.10



#ifndef _SOCKSTREAM_H
#define	_SOCKSTREAM_H

#ifdef WIN32
typedef int socklen_t;
typedef unsigned long in_addr_t;
#else
#include <sockconfig.h>
#if defined(_linux_) && (! defined(IPPROTO_IP))
typedef uint32_t in_addr_t;
#endif
#endif

// Uses ANSI C++ IOStream classes
#include <iostream>
using namespace std;
#include <stddef.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#else
#include <winsock.h>
#endif


#include "SWIutilLogger.hpp"

#ifndef _G_config_h

// non libg++ or g++ compilation

#       define _S_NOLIBGXX
#       define _S_USER_BUF		0x0001
#       define _S_UNBUFFERED		0x0002
#       define _S_NO_READS		0x0004
#       define _S_NO_WRITES		0x0008
#       define _S_EOF_SEEN		0x0010
#       define _S_ERR_SEEN		0x0020
#       define _S_DELETE_DONT_CLOSE	0x0040
#       define _S_LINKED		0x0080
#       define _S_LINE_BUF		0x0200
#       define _G_ssize_t size_t
#else
#       ifndef _S_USER_BUF
		// libg++ 2.5.x
#               define _S_USER_BUF		_IO_USER_BUF
#		define _S_UNBUFFERED		_IO_UNBUFFERED
#		define _S_NO_READS		_IO_NO_READS
#		define _S_NO_WRITES		_IO_NO_WRITES
#		define _S_EOF_SEEN		_IO_EOF_SEEN
#		define _S_ERR_SEEN		_IO_ERR_SEEN
#		define _S_DELETE_DONT_CLOSE	_IO_DELETE_DONT_CLOSE
#		define _S_LINKED		_IO_LINKED
#		define _S_LINE_BUF		_IO_LINE_BUF
#	endif //  !_S_USER_BUF


#endif //  !_G_config_h


#ifdef __linux__
// linux leaves these things out. We define it for compatitbility
// not for their use.
#ifndef SO_ACCEPTCONN
#	define SO_ACCEPTCONN	0x0002
#endif
#ifndef SO_USELOOPBACK
#	define SO_USELOOPBACK	0x0040
#endif
#ifndef SO_SNDLOWAT
#	define SO_SNDLOWAT	0x1003
#	define SO_RCVLOWAT	0x1004
#	define SO_SNDTIMEO	0x1005
#	define SO_RCVTIMEO	0x1006
#endif
#	define MSG_MAXIOVLEN	16
#ifndef SOMAXCONN
#	define SOMAXCONN	5
#endif
#endif // __linux__


#define SOCK_ERROR(classname, msg) \
{ \
  if (errno) \
    Error(500, L"%s%S%s%S%s%S%s%d", L"class", classname, L"msg", msg, \
	  L"errnoStr", strerror(errno), L"errno", errno); \
  else \
    Error(500, L"%s%S%s%S", L"class", classname, L"msg", msg); \
}

typedef int sockexcept;

#if defined(_WIN32) || (defined(__GNUC__) && (__GNUC__ > 2))
#define underflow_ret_t int
#else
#if defined(_decunix_)
#define underflow_ret_t streambuf::int_type
#else
#define underflow_ret_t unsigned int
#endif
#endif

struct sockaddr;

class SWIUTIL_API_CLASS sockAddr : public SWIutilLogger
{
 public:
  sockAddr(const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	   VXIunsigned diagTagBase)
    : SWIutilLogger (moduleName, pVXILog, diagTagBase) {}
  virtual ~sockAddr() {}
  virtual operator void*	() const =0;
  operator sockaddr* () const { return addr (); }
  virtual socklen_t	size 		() const =0;
  virtual int		family		() const =0;
  virtual sockaddr*   addr 		() const =0;
};

struct msghdr;

class SWIUTIL_API_CLASS sockbuf: public streambuf, public SWIutilLogger
{
 public:

  enum type {
    sock_stream	= SOCK_STREAM,
    sock_dgram	= SOCK_DGRAM,
    sock_raw	= SOCK_RAW,
    sock_rdm	= SOCK_RDM,
    sock_seqpacket  = SOCK_SEQPACKET
  };

  enum option {
    so_debug	= SO_DEBUG,
    so_acceptconn	= SO_ACCEPTCONN,
    so_reuseaddr	= SO_REUSEADDR,
    so_keepalive	= SO_KEEPALIVE,
    so_dontroute	= SO_DONTROUTE,
    so_broadcast	= SO_BROADCAST,
    so_useloopback	= SO_USELOOPBACK,
    so_linger	= SO_LINGER,
    so_oobinline	= SO_OOBINLINE,
    so_sndbuf	= SO_SNDBUF,
    so_rcvbuf	= SO_RCVBUF,
    so_sndlowat	= SO_SNDLOWAT,
    so_rcvlowat	= SO_RCVLOWAT,
    so_sndtimeo	= SO_SNDTIMEO,
    so_rcvtimeo	= SO_RCVTIMEO,
    so_error	= SO_ERROR,
    so_type		= SO_TYPE
  };

  enum level { sol_socket = SOL_SOCKET };

  enum msgflag {
    msg_oob		= MSG_OOB,
    msg_peek	= MSG_PEEK,
    msg_dontroute	= MSG_DONTROUTE,
    msg_maxiovlen	= MSG_MAXIOVLEN
  };

  enum shuthow {
    shut_read,
    shut_write,
    shut_readwrite
  };

  enum { somaxconn	= SOMAXCONN };

  struct socklinger {

    int	l_onoff;	// option on/off
    int	l_linger;	// linger time

    socklinger (int a, int b): l_onoff (a), l_linger (b) {}
  };

protected:

  struct sockcnt {

    SOCKET sock;

    int	cnt;

    sockcnt(SOCKET s, int c): sock(s), cnt(c) {}

  };

  sockcnt*	rep;  // counts the # refs to sock
  int		stmo; // -1==block, 0==poll, >0 == waiting time in secs
  int		rtmo; // -1==block, 0==poll, >0 == waiting time in secs

  virtual underflow_ret_t underflow ();
  virtual sockbuf::int_type  overflow (sockbuf::int_type c = EOF);
  virtual int		doallocate ();
  int			flush_output ();

#if defined(_WIN32) || (defined(__GNUC__)) || (defined(_decunix_))
  // backward compatible methods from old <streambuf.h> but missing from <streambuf>
  char*	base() { return eback(); }
  void    setb(char* b, char* eb, int a =0) { setbuf(b, eb-b); }
  int unbuffered() { return (egptr() <= base()+1) && (epptr() == pbase()); }
#endif


#if defined(_S_NOLIBGXX) || (defined(__GNUC__) && (__GNUC__ > 1))

  int         x_flags; // port to USL iostream
  int                 xflags () const { return x_flags; }
  int                 xsetflags (int f) { return x_flags |= f; }
  int                 xflags (int f)
  { int ret = x_flags; x_flags = f; return ret; }
  void                xput_char (char c) { *pptr() = c; pbump (1); }
  int                 linebuffered () const { return x_flags & _S_LINE_BUF; }
#endif // _S_NOLIBGXX

 public:
  sockbuf (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	   VXIunsigned diagTagBase, SOCKET soc = INVALID_SOCKET);
  sockbuf (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	   VXIunsigned diagTagBase, int domain, type st, int proto=0);
  sockbuf (const sockbuf&);
  sockbuf&		operator = (const sockbuf&);
  virtual 		~sockbuf ();
  operator int () const { return rep->sock; }


  virtual sockbuf*	open	(type, int proto=0);
  virtual sockbuf*	close	();

  virtual int		sync	();

  virtual _G_ssize_t	sys_read (char* buf, _G_ssize_t len);
  virtual _G_ssize_t	sys_write (const void* buf, long len);

  virtual int		xsputn (const char* s, int n);

  int			is_open () const { return rep->sock != INVALID_SOCKET; }
  int			is_eof  ()       { return xflags() & _S_EOF_SEEN; }

  virtual int		bind	(sockAddr&);
  virtual int		connect	(sockAddr&);

  int		listen	(int num=somaxconn);
  virtual sockbuf	accept	();
  virtual sockbuf	accept	(sockAddr& sa);

  int			read	(void* buf, int len);
  int			recv	(void* buf, int len, int msgf=0);
  int			recvfrom(sockAddr& sa,void* buf, int len, int msgf=0);

#if !defined(__linux__) && !defined(_WIN32)
  int			recvmsg (msghdr* msg, int msgf=0);
  int			sendmsg	(msghdr* msg, int msgf=0);
#endif


  int			write	(const void* buf, int len);
  int			send	(const void* buf, int len, int msgf=0);
  int			sendto	(sockAddr& sa,const void* buf, int len, int msgf=0);

  int			sendtimeout (int wp=-1);
  int			recvtimeout (int wp=-1);
  int			is_readready (int wp_sec, int wp_usec=0) const;
  int			is_writeready (int wp_sec, int wp_usec=0) const;
  int			is_exceptionpending (int wp_sec, int wp_usec=0) const;

  void		shutdown (shuthow sh);
  int			getopt(option op, void* buf,int len,
			       level l=sol_socket) const;

  int           setBlocking(bool blocking);

  void		setopt(option op, void* buf,int len,
                       level l=sol_socket) const;

  type		gettype () const;

  int			clearerror () const;
  int			debug	  (int opt= -1) const;
  int			reuseaddr (int opt= -1) const;
  int			keepalive (int opt= -1) const;
  int			dontroute (int opt= -1) const;
  int			broadcast (int opt= -1) const;
  int			oobinline (int opt= -1) const;
  int			linger    (int tim= -1) const;
  int			sendbufsz (int sz=-1)   const;
  int			recvbufsz (int sz=-1)   const;

  void		error (const char* errmsg) const;
};


class SWIUTIL_API_CLASS isockstream: public istream
{
 protected:
  isockstream ():
    istream (0)
  {}

 public:
  isockstream(sockbuf* sb):
    istream (sb)
  {}

  virtual ~isockstream ();

  sockbuf*		rdbuf () { return static_cast<sockbuf*>(ios::rdbuf());}
  sockbuf*		operator -> () { return rdbuf(); }
};


class SWIUTIL_API_CLASS osockstream: public ostream
{
 protected:
  osockstream():
    ostream (0)
  {}

 public:
  osockstream(sockbuf* sb):
    ostream (sb)
  {}

  virtual ~osockstream ();

  sockbuf*		rdbuf () { return static_cast<sockbuf*>(ios::rdbuf());}
  sockbuf*		operator -> () { return rdbuf(); }
};


class SWIUTIL_API_CLASS iosockstream: public iostream
{
 protected:
  iosockstream ():
    iostream (0)
  {}

 public:
  iosockstream(sockbuf *sb):
    iostream (sb)
  {}

  virtual ~iosockstream ();

  sockbuf*		rdbuf () { return static_cast<sockbuf*>(ios::rdbuf());}
  sockbuf*		operator -> () { return rdbuf(); }
};


#endif	// _SOCKSTREAM_H
