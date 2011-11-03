// sockstream.C -*- C++ -*- socket library
// Copyright (C) 1992,1993,1994 Gnanasekaran Swaminathan <gs4t@virginia.edu>
//
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided the above copyright
// notice and this paragraph are preserved on all copies.
// This software is provided "as is" with no express or implied warranty.
//
// Version: 17Oct95 1.10
//
// You can simultaneously read and write into
// a sockbuf just like you can listen and talk
// through a telephone. Hence, the read and the
// write buffers are different. That is, they do not
// share the same memory.
//
// Read:
// gptr() points to the start of the get area.
// The unread chars are gptr() - egptr().
// base() points to the read buffer
//
// eback() is set to base() so that pbackfail()
// is called only when there is no place to
// putback a char. And pbackfail() always returns EOF.
//
// Write:
// pptr() points to the start of the put area
// The unflushed chars are pbase() - pptr()
// pbase() points to the write buffer.
// epptr() points to the end of the write buffer.
//
// Output is flushed whenever one of the following conditions
// holds:
// (1) pptr() == epptr()
// (2) EOF is written
// (3) linebuffered and '\n' is written
//
// Unbuffered:
// Input buffer size is assumed to be of size 1 and output
// buffer is of size 0. That is, egptr() <= base()+1 and
// epptr() == pbase().


#include "SWIutilInternal.h"

#include "sockstream.h"

#include <string.h>

#ifndef _WIN32
EXTERN_C_BEGIN
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#ifdef _solaris_
#include <sys/filio.h>
#endif
EXTERN_C_END
#else
#include <winsock.h>
#ifdef EINTR
#undef EINTR
#endif
#define EINTR WSAEINTR
#endif /* !_WIN32 */

#ifndef BUFSIZ
#  define BUFSIZ 1024
#endif

#ifdef FD_ZERO
#  undef FD_ZERO    // bzero causes so much trouble to us
#endif
#define FD_ZERO(p) (memset ((p), 0, sizeof *(p)))

#define LOG_ERROR(_err, _func) Error(_err, L"%s%s", L"Func", _func);
#ifdef _WIN32
#define LOG_ERROR_DETAILED(_err, _func) \
{ \
  int wserr = WSAGetLastError(); \
  Error(_err, L"%s%s%s%i", L"Func", _func, L"WinsockError", wserr); \
}
#else
#define LOG_ERROR_DETAILED(_err, _func) LOG_ERROR(_err, _func)
#endif

sockbuf::sockbuf (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		  VXIunsigned diagTagBase, SOCKET soc)
  : streambuf(), SWIutilLogger(moduleName, pVXILog, diagTagBase),
    rep (new sockcnt (soc, 1)), stmo (-1), rtmo (-1)
{
  xflags (0);
  xsetflags (_S_LINE_BUF);
}

sockbuf::sockbuf (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		  VXIunsigned diagTagBase, int domain, sockbuf::type st,
		  int proto)
  : streambuf(), SWIutilLogger(moduleName, pVXILog, diagTagBase),
    rep (0), stmo (-1), rtmo (-1)
{
  SOCKET soc = ::socket (domain, st, proto);
  rep = new sockcnt (soc, 1);
  xflags (0);
  if ((! rep) || (rep->sock == INVALID_SOCKET)) 
    error ("invalid socket");
  xsetflags (_S_LINE_BUF);
}

sockbuf::sockbuf (const sockbuf& sb)
  : streambuf(), SWIutilLogger(sb),
    rep (sb.rep), stmo (sb.stmo), rtmo (sb.rtmo)
{
  xflags (0);
  rep->cnt++;
  xsetflags (_S_LINE_BUF);
}

sockbuf& sockbuf::operator = (const sockbuf& sb)
{
  if (this != &sb && rep != sb.rep && rep->sock != sb.rep->sock) {
    this->sockbuf::~sockbuf();
    rep  = sb.rep; stmo = sb.stmo; rtmo = sb.rtmo; 
    SWIutilLogger::Create(sb.GetModuleName(), sb.GetLog(), sb.GetDiagBase());
    rep->cnt++;
#ifdef _S_NOLIBGXX
    xflags (sb.xflags ());
#else
    // xflags () is a non-const member function in libg++.
    xflags (((sockbuf&)sb).xflags ());
#endif
  }
  return *this;
}

sockbuf::~sockbuf ()
{
  overflow (EOF);
  if (rep->cnt == 1 && !(xflags () & _S_DELETE_DONT_CLOSE)) close ();
  delete [] base ();
  if (--rep->cnt == 0) delete rep;
}

sockbuf* sockbuf::open (type, int)
{
  return 0;
}

sockbuf* sockbuf::close()
{
#ifdef _WIN32
  if (rep->sock != INVALID_SOCKET)
  {
     closesocket(rep->sock);
     rep->sock = INVALID_SOCKET;
  }
#else
  if (rep->sock >= 0)
  {
    if (::close (rep->sock) == -1) return this;
    rep->sock = -1;
  }
#endif
  return 0;
}

_G_ssize_t sockbuf::sys_read (char* buf, _G_ssize_t len)
// return EOF on eof, 0 on timeout, and # of chars read on success
{
  return read (buf, len);
}

_G_ssize_t sockbuf::sys_write (const void* buf, long len)
// return written_length; < len indicates error
{
  return write (buf, (int) len);
}

int sockbuf::flush_output()
// return 0 when there is nothing to flush or when the flush is a success
// return EOF when it could not flush
{
  if (pptr () <= pbase ()) return 0;
  if (!(xflags () & _S_NO_WRITES)) {
    int wlen   = pptr () - pbase ();
    int wval   = sys_write (pbase (), wlen);
    int status = (wval == wlen)? 0: EOF;
#ifndef _WIN32
    if (unbuffered()) setp (pbase (), pbase ());
    else
#endif
		setp (pbase (), pbase () + BUFSIZ);
    return status;
  }
  return EOF;
}

int sockbuf::sync ()
{
  return flush_output ();
}

int sockbuf::doallocate ()
// return 1 on allocation and 0 if there is no need
{
  if (!base ()) {
    char*	buf = new char[2*BUFSIZ];
    setb (buf, buf+BUFSIZ);
    setg (buf, buf, buf);

    buf += BUFSIZ;
    setp (buf, buf+BUFSIZ);
    return 1;
  }
  return 0;
}

underflow_ret_t sockbuf::underflow ()
{
  if (xflags () & _S_NO_READS) return EOF;

  if (gptr () < egptr ()) return *(unsigned char*)gptr ();

  if (base () == 0 && doallocate () == 0) return EOF;

  int bufsz = unbuffered () ? 1: BUFSIZ;
  int rval = sys_read (base (), bufsz);
  if (rval == EOF) {
    xsetflags (_S_EOF_SEEN);
    return EOF;
  }else if (rval == 0)
    return EOF;
  setg (eback (), base (), base () + rval);
  return *(unsigned char*)gptr ();
}

sockbuf::int_type sockbuf::overflow (sockbuf::int_type c)
// if c == EOF, return flush_output();
// if c == '\n' and linebuffered, insert c and
// return (flush_output()==EOF)? EOF: c;
// otherwise insert c into the buffer and return c
{
  if (c == EOF) return flush_output ();

  if (xflags () & _S_NO_WRITES) return EOF;

  if (pbase () == 0 && doallocate () == 0) return EOF;

  if (pptr () >= epptr() && flush_output () == EOF)
    return EOF;

  xput_char (c);
  if ((unbuffered () || linebuffered () && c == '\n' || pptr () >= epptr ())
      && flush_output () == EOF)
    return EOF;

  return c;
}

int sockbuf::xsputn (const char* s, int n)
{
  if (n <= 0) return 0;
  const unsigned char* p = (const unsigned char*)s;

  for (int i=0; i<n; i++, p++) {
    if (*p == '\n') {
      if (overflow (*p) == EOF) return i;
    } else
      if (sputc (*p) == EOF) return i;
  }
  return n;
}

int sockbuf::bind (sockAddr& sa)
{
  return ::bind (rep->sock, sa.addr (), sa.size ());
}

int sockbuf::connect (sockAddr& sa)
{
  if (::connect(rep->sock, sa.addr (), sa.size()) != 0) {
#ifdef _WIN32
    int wserr = WSAGetLastError();
    if (wserr != WSAEWOULDBLOCK) {
      Error(500, L"%s%s%s%i", L"Func", L"connect", L"WinsockError", wserr);
      return -1;
    }
#else
    if (errno != EINPROGRESS) {
      Error(500, L"%s%s%s%i", L"Func", L"connect", L"errno", errno);
      return -1;
    }
#endif
  }
  return 0;
}

int sockbuf::listen (int num)
{
  return ::listen (rep->sock, num);
}

sockbuf	sockbuf::accept (sockAddr& sa)
{
  socklen_t len = sa.size ();
  SOCKET soc = INVALID_SOCKET;
  while ((soc = ::accept (rep->sock, sa.addr (), &len)) == INVALID_SOCKET
	 && errno == EINTR);
  return sockbuf(GetModuleName(), GetLog(), GetDiagBase(), soc);
}

sockbuf	sockbuf::accept ()
{
  SOCKET soc = INVALID_SOCKET;
  while ((soc = ::accept (rep->sock, 0, 0)) == INVALID_SOCKET
	 && errno == EINTR);
  return sockbuf(GetModuleName(), GetLog(), GetDiagBase(), soc);
}

int sockbuf::read (void* buf, int len)
{
#ifdef _WIN32
  return recv (buf, len); // Winsock uses recv
#else
  if (rtmo != -1 && is_readready (rtmo)==0) return 0;

  int	rval;
  if ((rval = ::read (rep->sock, (char*) buf, len)) == -1)
    error("read");
  return (rval==0) ? EOF: rval;
#endif // _WIN32
}

int sockbuf::recv (void* buf, int len, int msgf)
{
  if (rtmo != -1 && is_readready (rtmo)==0) return 0;

  int	rval;
  if ((rval = ::recv (rep->sock, (char*) buf, len, msgf)) == -1)
    LOG_ERROR_DETAILED (500, L"recv");
  return (rval==0) ? EOF: rval;
}

int sockbuf::recvfrom (sockAddr& sa, void* buf, int len, int msgf)
{
  if (rtmo != -1 && is_readready (rtmo)==0) return 0;

  int rval;
  socklen_t sa_len = sa.size ();

  if ((rval = ::recvfrom (rep->sock, (char*) buf, len,
			  msgf, sa.addr (), &sa_len)) == -1)
    LOG_ERROR_DETAILED (500, L"recvfrom");
  return (rval==0) ? EOF: rval;
}

int sockbuf::write(const void* buf, int len)
{
#ifdef _WIN32
  return send(buf, len); // Winsock uses send
#else
  if (stmo != -1 && is_writeready (stmo)==0) return 0;

  int	wlen=0;
  while(len>0) {
    int	wval;
    if ((wval = ::write (rep->sock, (char*) buf, len)) == -1) {
      error ("sockbuf::write");
      return wval;
    }
    len -= wval;
    wlen += wval;
  }
  return wlen; // == len if every thing is all right
#endif // _WIN32
}

int sockbuf::send (const void* buf, int len, int msgf)
{
  if (stmo != -1 && is_writeready (stmo)==0) return 0;

  int	wlen=0;
  while(len>0) {
    int	wval;
    if ((wval = ::send (rep->sock, (char*) buf, len, msgf)) == -1) {
      LOG_ERROR_DETAILED (500, L"send");
      return wval;
    }
    len -= wval;
    wlen += wval;
  }
  return wlen;
}

int sockbuf::sendto (sockAddr& sa, const void* buf, int len, int msgf)
{
  if (stmo != -1 && is_writeready (stmo)==0) return 0;

  int	wlen=0;
  while(len>0) {
    int	wval;
    if ((wval = ::sendto (rep->sock, (char*) buf, len,
			  msgf, sa.addr (), sa.size())) == -1) {
      LOG_ERROR_DETAILED (500, L"sendto");
      return wval;
    }
    len -= wval;
    wlen += wval;
  }
  return wlen;
}

#if !defined (__linux__) && !defined(_WIN32)
// linux does not have sendmsg or recvmsg

int sockbuf::recvmsg (msghdr* msg, int msgf)
{
  if (rtmo != -1 && is_readready (rtmo)==0) return 0;

  int	rval;
  if ((rval = ::recvmsg(rep->sock, msg, msgf)) == -1)
    error ("sockbuf::recvmsg");
  return (rval==0)? EOF: rval;
}

int sockbuf::sendmsg (msghdr* msg, int msgf)
{
  if (stmo != -1 && is_writeready (stmo)==0) return 0;

  int	wval;
  if ((wval = ::sendmsg (rep->sock, msg, msgf)) == -1)
    error("sendmsg");
  return wval;
}

#endif //!__linux__

int sockbuf::sendtimeout (int wp)
{
  int oldstmo = stmo;
  stmo = (wp < 0) ? -1: wp;
  return oldstmo;
}

int sockbuf::recvtimeout (int wp)
{
  int oldrtmo = rtmo;
  rtmo = (wp < 0) ? -1: wp;
  return oldrtmo;
}

int sockbuf::is_readready (int wp_sec, int wp_usec) const
{
  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET (rep->sock, &rfds);

  fd_set efds;
  FD_ZERO (&efds);
  FD_SET (rep->sock, &efds);

  timeval tv;
  tv.tv_sec  = wp_sec;
  tv.tv_usec = wp_usec;

  int ret = select (rep->sock+1, &rfds, 0, &efds, (wp_sec == -1) ? 0: &tv);

  if (ret < 0) 
  {
    LOG_ERROR_DETAILED (500, L"select");
    return ret;
  }
  else if (ret == 0)
  {
    LOG_ERROR (501, L"select");
    return ret;
  }

  if (FD_ISSET(rep->sock, &efds))
  {
    LOG_ERROR (502, L"select");
    return -1;
  }

  return FD_ISSET(rep->sock,&rfds);
}

int sockbuf::is_writeready (int wp_sec, int wp_usec) const
{
  fd_set wfds;
  FD_ZERO (&wfds);
  FD_SET (rep->sock, &wfds);

  fd_set efds;
  FD_ZERO (&efds);
  FD_SET (rep->sock, &efds);

  timeval tv;
  tv.tv_sec  = wp_sec;
  tv.tv_usec = wp_usec;

  int ret = select (rep->sock+1, 0, &wfds, &efds, (wp_sec == -1) ? 0: &tv);

  if (ret < 0) 
  {
    LOG_ERROR_DETAILED (500, L"select");
    return ret;
  }
  else if (ret == 0)
  {
    LOG_ERROR (501, L"select");
    return ret;
  }

  if (FD_ISSET(rep->sock, &efds))
  {
    LOG_ERROR (502, L"select");
    return -1;
  }

  return FD_ISSET(rep->sock,&wfds);
}

int sockbuf::is_exceptionpending (int wp_sec, int wp_usec) const
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET  (rep->sock, &fds);

  timeval tv;
  tv.tv_sec = wp_sec;
  tv.tv_usec = wp_usec;

  int ret = select (rep->sock+1, 0, 0, &fds, (wp_sec == -1) ? 0: &tv);
  if (ret == -1) {
    LOG_ERROR_DETAILED (500, L"select");
    return 0;
  }
  return ret;
}

void sockbuf::shutdown (shuthow sh)
{
  switch (sh) {
  case shut_read:
    xsetflags(_S_NO_READS);
    break;
  case shut_write:
    xsetflags(_S_NO_WRITES);
    break;
  case shut_readwrite:
    xsetflags(_S_NO_READS|_S_NO_WRITES);
    break;
  }
  if (::shutdown(rep->sock, sh) == INVALID_SOCKET)
    error("shutdown");
}

int sockbuf::getopt (option op, void* buf, int len, level l) const
{
  socklen_t rlen = len;
  if (::getsockopt (rep->sock, l, op, (char*) buf, &rlen) == -1)
    error ("sockbuf::getopt");
  return rlen;
}

int sockbuf::setBlocking(bool blocking)
{
  unsigned long x = (unsigned long) !blocking;
#ifdef _WIN32
  return ioctlsocket(rep->sock, FIONBIO, &x);
#else
  return ioctl(rep->sock, FIONBIO, &x);
#endif
}

void sockbuf::setopt (option op, void* buf, int len, level l) const
{
  if (::setsockopt (rep->sock, l, op, (char*) buf, len) == -1)
    error ("sockbuf::setopt");
}

sockbuf::type sockbuf::gettype () const
{
  int	ty=0;
  getopt (so_type, &ty, sizeof (ty));
  return sockbuf::type(ty);
}

int sockbuf::clearerror () const
{
  int 	err=0;
  getopt (so_error, &err, sizeof (err));
  return err;
}

int sockbuf::debug (int opt) const
{
  int old=0;
  getopt (so_debug, &old, sizeof (old));
  if (opt != -1)
    setopt (so_debug, &opt, sizeof (opt));
  return old;
}

int sockbuf::reuseaddr (int opt) const
{
  int old=0;
  getopt (so_reuseaddr, &old, sizeof (old));
  if (opt != -1)
    setopt (so_reuseaddr, &opt, sizeof (opt));
  return old;
}

int sockbuf::keepalive (int opt) const
{
  int old=0;
  getopt (so_keepalive, &old, sizeof (old));
  if (opt != -1)
    setopt (so_keepalive, &opt, sizeof (opt));
  return old;
}

int sockbuf::dontroute (int opt) const
{
  int old=0;
  getopt (so_dontroute, &old, sizeof (old));
  if (opt != -1)
    setopt (so_dontroute, &opt, sizeof (opt));
  return old;
}

int sockbuf::broadcast (int opt) const
{
  int old=0;
  getopt (so_broadcast, &old, sizeof (old));
  if (opt != -1)
    setopt (so_broadcast, &opt, sizeof (opt));
  return old;
}

int sockbuf::oobinline (int opt) const
{
  int old=0;
  getopt (so_oobinline, &old, sizeof (old));
  if (opt != -1)
    setopt (so_oobinline, &opt, sizeof (opt));
  return old;
}

int sockbuf::linger (int opt) const
{
  socklinger	old (0, 0);
  getopt (so_linger, &old, sizeof (old));
  if (opt > 0) {
    socklinger nw (1, opt);
    setopt (so_linger, &nw, sizeof (nw));
  }else if (opt == 0) {
    socklinger nw (0, old.l_linger);
    setopt (so_linger, &nw, sizeof (nw));
  }
  return old.l_onoff ? old.l_linger: -1;
}

int sockbuf::sendbufsz (int  sz) const
{
  int old=0;
  getopt (so_sndbuf, &old, sizeof (old));
  if (sz >= 0)
    setopt (so_sndbuf, &sz, sizeof (sz));
  return old;
}

int sockbuf::recvbufsz (int sz) const
{
  int old=0;
  getopt (so_rcvbuf, &old, sizeof (old));
  if (sz >= 0)
    setopt (so_rcvbuf, &sz, sizeof (sz));
  return old;
}

void sockbuf::error (const char* msg) const
{
  SOCK_ERROR("sockbuf", msg);
}

isockstream::~isockstream ()
{
}

osockstream::~osockstream ()
{
}

iosockstream::~iosockstream ()
{
}
