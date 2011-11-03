
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Vocalocity, the Vocalocity logo, and VocalOS are trademarks or 
 * registered trademarks of Vocalocity, Inc. 
 * OpenVXI is a trademark of Scansoft, Inc. and used under license 
 * by Vocalocity.
 ***********************************************************************/

#if _MSC_VER >= 1100    // Visual C++ 5.x
#pragma warning( disable : 4786 4503 )
#endif

#ifdef _WIN32
#include <winsock2.h>
#define WSA_LAST_ERROR WSAGetLastError()
#define WSA_ERROR_NAME L"WinsockError"
#else
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#ifdef _solaris_
#include <stropts.h>
#include <fcntl.h>
#include <string.h>
#endif
#define WSA_LAST_ERROR errno
#define WSA_ERROR_NAME L"errno"
#define WSAEADDRINUSE EADDRINUSE
#define WSAECONNABORTED ECONNABORTED
#define WSAECONNRESET ECONNRESET
#define WSAECONNREFUSED ECONNREFUSED
#define WSAEHOSTDOWN EHOSTDOWN
#define WSAEHOSTUNREACH EHOSTUNREACH
#define WSAEWOULDBLOCK EINPROGRESS
#define WSAEACCESS EACCESS
#define WSAENOTSOCK EBADF
#define WSAEINTR EINTR
#define closesocket close
#define ioctlsocket ioctl
#endif
#include "SWIsocket.hpp"
#include "SWIipAddress.hpp"


static timeval* millisecToTimeval(long millisec, timeval& tv)
{
  if (millisec < 0)
  {
    tv.tv_sec  = -1;
    tv.tv_usec = -1;
    return NULL;
  }

  tv.tv_sec = (millisec / 1000);
  tv.tv_usec = (millisec % 1000) * 1000;
  return &tv;
}




SWIsocket::SWIsocket(Type socktype,
                     SWIutilLogger *logger):
  _logger(logger), _sd(INVALID_SOCKET),
  _bound(false), _connected(false), _shutIn(false), _shutOut(false),
  _remoteAddress(NULL), _localAddress(NULL), _rtmo(-1), _stmo(-1),
  _inStream(NULL), _outStream(NULL)
{
  _sd = socket(AF_INET, socktype, 0);
  if (_sd == INVALID_SOCKET)
  {
    error(L"socket", 500, WSA_LAST_ERROR);
  }
}

SWIsocket::SWIsocket(const SWIsocket *socket, SOCKET sd):
  _logger(socket->_logger), _sd(sd), _bound(true), _connected(true),
  _shutIn(false), _shutOut(false),
  _remoteAddress(NULL), _localAddress(NULL),
  _rtmo(socket->_rtmo), _stmo(socket->_stmo),
  _inStream(NULL), _outStream(NULL)
{}

SWIsocket::~SWIsocket()
{
  close();
  if (_inStream != NULL)
  {
    _inStream->_sock = NULL;
  }

  if (_outStream != NULL)
  {
    _outStream->_sock = NULL;
  }
  delete _remoteAddress;
  delete _localAddress;
}

SWIstream::Result SWIsocket::connect(const SWIipAddress& endpoint, long timeout)
{
  SWIstream::Result rc = SWIstream::SUCCESS;

  unsigned long nonblocking = 1;
  int flags = 0;
  timeval tv;

  timeval *ptv = millisecToTimeval(timeout, tv);

  if (ptv != NULL)
  {
#ifdef _solaris_
		flags = fcntl(_sd, F_GETFL, 0);
		flags  |= O_NONBLOCK;
		fcntl(_sd, F_SETFL, flags);
#else
    ioctlsocket(_sd, FIONBIO, &nonblocking);
    nonblocking = 0;
#endif
  }

  if (::connect(_sd, endpoint.addr(), endpoint.size()) != 0)
  {
    int wserr = WSA_LAST_ERROR;
    if (wserr != WSAEWOULDBLOCK)
    {
      error(L"connect", 500, wserr);
      rc = SWIstream::FAILURE;
      goto cleanup;
    }
  }

  if (ptv != NULL)
  {
    int status = is_writeready(ptv);
    if (status == 0)
    {
      rc = SWIstream::TIMED_OUT;
      goto cleanup;
    }
    if (status < 0)
    {
      rc = SWIstream::OPEN_ERROR;
      goto cleanup;
    }
  }

  _bound = _connected = true;
 cleanup:

  // restore blocking mode.
  if (ptv != NULL)
  {
#ifdef _solaris_
		flags = fcntl(_sd, F_GETFL, 0);
		flags &= ~O_NONBLOCK;
		fcntl(_sd, F_SETFL, flags);
#else
    ioctlsocket(_sd, FIONBIO, &nonblocking);
#endif
  }
  return rc;
}

SWIstream::Result SWIsocket::connect(const char *hostname, int port, long timeout)
{
  SWIipAddress endpoint(hostname, port, _logger);

  return connect(endpoint, timeout);
}


SWIstream::Result SWIsocket::connect(unsigned long addr, int port, long timeout)
{
  SWIipAddress endpoint(addr, port, _logger);
  return connect(endpoint, timeout);
}

SWIstream::Result SWIsocket::connect(unsigned long addr,
                                     const char *service_name,
                                     const char *protocol_name,
                                     long timeout)
{
  SWIipAddress endpoint(addr, service_name, protocol_name, _logger);
  return connect(endpoint, timeout);
}

SWIstream::Result SWIsocket::connect(const char *host,
                                     const char *service_name,
                                     const char *protocol_name,
                                     long timeout)
{
  SWIipAddress endpoint(host, service_name, protocol_name, _logger);
  return connect(endpoint, timeout);
}

SWIstream::Result SWIsocket::bind(const SWIipAddress& sa)
{
  int rc = ::bind(_sd, sa.addr (), sa.size ());
  if (rc != 0)
  {
    error(L"bind", 500, WSA_LAST_ERROR);
    return SWIstream::FAILURE;
  }
  _bound = true;
  return SWIstream::SUCCESS;
}

SWIstream::Result SWIsocket::bind()
{
  SWIipAddress sa(_logger);
  return bind(sa);
}

SWIstream::Result SWIsocket::bind(unsigned long addr, int port_no)
{
  SWIipAddress sa(addr, port_no, _logger);
  return bind(sa);
}

SWIstream::Result SWIsocket::bind(const char* host_name, int port_no)
{
  SWIipAddress sa(host_name, port_no, _logger);
  return bind (sa);
}

SWIstream::Result SWIsocket::bind(unsigned long addr,
                                  const char* service_name,
                                  const char* protocol_name)
{
  SWIipAddress sa(addr, service_name, protocol_name, _logger);
  return bind(sa);
}

SWIstream::Result SWIsocket::bind(const char* host_name,
                                  const char* service_name,
                                  const char* protocol_name)
{
  SWIipAddress sa(host_name, service_name, protocol_name, _logger);
  return bind(sa);
}


SWIstream::Result SWIsocket::listen(int backlog)
{
  int rc = ::listen (_sd, backlog);
  if (rc != 0)
  {
    error(L"listen", 500, WSA_LAST_ERROR);
    return SWIstream::FAILURE;
  }
  return SWIstream::SUCCESS;
}

SWIsocket *SWIsocket::accept(SWIipAddress& sa)
{
  int wserr;
  socklen_t len = sa.size();
  SOCKET soc = INVALID_SOCKET;
  while ((soc = ::accept (_sd, sa.addr(), &len)) == INVALID_SOCKET
	 && (wserr = WSA_LAST_ERROR) == WSAEINTR);
  if (soc == INVALID_SOCKET)
  {
    error(L"accept", 500, wserr);
    return NULL;
  }
  return new SWIsocket(this, soc);
}

SWIsocket* SWIsocket::accept()
{
  int wserr;
  SOCKET soc = INVALID_SOCKET;
  while ((soc = ::accept (_sd, NULL, NULL)) == INVALID_SOCKET
	 && (wserr = WSA_LAST_ERROR) == WSAEINTR);
  if (soc == INVALID_SOCKET)
  {
    error(L"accept", 500, wserr);
    return NULL;
  }
  return new SWIsocket(this, soc);
}


const SWIipAddress *SWIsocket::getRemoteAddress() const
{
  //   if (!_connected)
  //     return NULL;

  if (_remoteAddress != NULL)
    return _remoteAddress;

  _remoteAddress = new SWIipAddress(_logger);

  socklen_t len = _remoteAddress->size();

  if (::getpeername(_sd, _remoteAddress->addr(), &len) != 0)
  {
    error(L"getpeername", 500, WSA_LAST_ERROR);
    delete _remoteAddress;
    _remoteAddress = NULL;
  }
  return _remoteAddress;
}

const SWIipAddress *SWIsocket::getLocalAddress() const
{
  //if (!_bound)
  //return NULL;

  if (_localAddress != NULL)
    return _localAddress;

  _localAddress = new SWIipAddress(_logger);

  socklen_t len = _localAddress->size();

  if (::getsockname(_sd, _localAddress->addr(), &len) != 0)
  {
    error(L"getsockname", 500, WSA_LAST_ERROR);
    delete _localAddress;
    _localAddress = NULL;
  }

  return _localAddress;
}


int SWIsocket::getRemotePort() const
{
  const SWIipAddress* addr = getRemoteAddress();
  if (addr == NULL)
  {
    return 0;
  }
  else
    return addr->getport();
}

int SWIsocket::getLocalPort() const
{
  const SWIipAddress* addr = getLocalAddress();
  if (addr == NULL)
  {
    return 0;
  }
  else
    return addr->getport();
}

SWIstream::Result SWIsocket::close()
{
  if (_sd != INVALID_SOCKET)
  {
    ::closesocket(_sd);
    _sd = INVALID_SOCKET;
  }
  return SWIstream::SUCCESS;
}


SWIinputStream *SWIsocket::getInputStream()
{
  if (_inStream == NULL)
  {
    _inStream = new InputStream(this);
  }
  return _inStream;
}

SWIoutputStream *SWIsocket::getOutputStream()
{
  if (_outStream == NULL)
  {
    _outStream = new OutputStream(this);
  }

  return _outStream;
}

SWIsocket::InputStream::InputStream(SWIsocket *sock):
  _sock(sock)
{}

SWIsocket::InputStream::~InputStream()
{
  if (_sock != NULL)
  {
    _sock->shutdown(shut_read);
    _sock->_inStream = NULL;
  }
}

int SWIsocket::InputStream::readBytes(void *data, int dataSize)
{
  return _sock != NULL ? _sock->recv(data, dataSize) : SWIstream::ILLEGAL_STATE;
}

SWIstream::Result SWIsocket::InputStream::close()
{
  if (_sock == NULL)
    return SWIstream::ILLEGAL_STATE;

  return _sock->shutdown(shut_read);
}

SWIstream::Result SWIsocket::InputStream::waitReady(long timeoutMs)
{
  if (_sock == NULL)
    return SWIstream::ILLEGAL_STATE;

  switch (_sock->is_readready(timeoutMs))
  {
   case 1:
     return SUCCESS;
   case 0:
     return TIMED_OUT;
   default:
     return FAILURE;
  }
}

SWIsocket::OutputStream::OutputStream(SWIsocket *sock):
  _sock(sock)
{}

SWIsocket::OutputStream::~OutputStream()
{
  if (_sock != NULL)
  {
    _sock->shutdown(shut_write);
    _sock->_outStream = NULL;
  }
}

int SWIsocket::OutputStream::writeBytes(const void *buffer, int bufferSize)
{
  return _sock != NULL ? _sock->send(buffer, bufferSize) : SWIstream::ILLEGAL_STATE;
}

SWIstream::Result SWIsocket::OutputStream::close()
{
  if (_sock == NULL)
    return SWIstream::ILLEGAL_STATE;

  return _sock->shutdown(shut_write);
}

SWIstream::Result SWIsocket::OutputStream::waitReady(long timeoutMs)
{
  if (_sock == NULL)
    return SWIstream::ILLEGAL_STATE;

  switch (_sock->is_writeready(timeoutMs))
  {
   case 1:
     return SUCCESS;
   case 0:
     return TIMED_OUT;
   default:
     return FAILURE;
  }
}

int SWIsocket::read(void* buf, int len)
{
#ifdef _WIN32
  return recv(buf, len); // Winsock uses recv
#else
  if (_rtmo != -1 && is_readready(_rtmo) == 0)
    return SWIstream::TIMED_OUT;

  int rval = ::read(_sd, (char *) buf, len);

  switch (rval)
  {
   case -1:
     {
       int wserr = WSA_LAST_ERROR;
       if (wserr == WSAECONNABORTED || wserr == WSAECONNRESET)
       {
         rval = SWIstream::CONNECTION_ABORTED;
       }
       else
       {
         error(L"read", 500, wserr);
         rval = SWIstream::READ_ERROR;
       }
       break;
     }
   case 0:
     rval = SWIstream::END_OF_FILE;
     break;
   default:
     break;
  }

  return rval;
#endif // _WIN32
}

int SWIsocket::recv(void* buf, int len, int msgf)
{
  if (_rtmo != -1 && is_readready (_rtmo)==0)
    return SWIstream::TIMED_OUT;

  int rval = ::recv(_sd, (char*) buf, len, msgf);

  switch (rval)
  {
   case -1:
     {
       int wserr = WSA_LAST_ERROR;
       if (wserr == WSAECONNABORTED || wserr == WSAECONNRESET)
       {
         rval = SWIstream::CONNECTION_ABORTED;
       }
       else
       {
         error(L"recv", 500, wserr);
         rval = SWIstream::READ_ERROR;
       }
     }
     break;
   case 0:
     rval = SWIstream::END_OF_FILE;
     break;
   default:
     break;
  }

  return rval;
}

int SWIsocket::recvfrom (SWIipAddress& sa, void* buf, int len, int msgf)
{
  if (_rtmo != -1 && is_readready (_rtmo)==0)
    return SWIstream::TIMED_OUT;

  socklen_t sa_len = sa.size ();
  int rval = ::recvfrom(_sd, (char*) buf, len,
                        msgf, sa.addr(), &sa_len);

  switch (rval)
  {
   case -1:
     error(L"recvfrom", 500, WSA_LAST_ERROR);
     rval = SWIstream::READ_ERROR;
     break;
   case 0:
     rval = SWIstream::END_OF_FILE;
     break;
   default:
     break;
  }

  return rval;
}

int SWIsocket::write(const void* buf, int len)
{
#ifdef _WIN32
  return send(buf, len); // Winsock uses send
#else
  if (_stmo != -1 && is_writeready (_stmo)==0) return 0;

  int wlen = 0;
  while (len > 0)
  {
    int	wval;
    if ((wval = ::write (_sd, (char*) buf, len)) == -1)
    {
      error(L"write", 500, WSA_LAST_ERROR);
      return SWIstream::WRITE_ERROR;
    }
    len -= wval;
    wlen += wval;
  }
  return wlen; // == len if every thing is all right
#endif // _WIN32
}

int SWIsocket::send (const void* buf, int len, int msgf)
{
  if (_stmo != -1 && is_writeready (_stmo)==0) return 0;

  int	wlen=0;
  while(len>0) {
    int	wval;
    if ((wval = ::send (_sd, (char*) buf, len, msgf)) == -1)
    {
      int wserr = WSA_LAST_ERROR;
      if (wserr == WSAECONNRESET || wserr == WSAECONNABORTED)
      {
        return SWIstream::CONNECTION_ABORTED;
      }
      else
      {
        error(L"send", 500, WSA_LAST_ERROR);
        return SWIstream::WRITE_ERROR;
      }
    }
    len -= wval;
    wlen += wval;
  }
  return wlen;
}

int SWIsocket::sendto(const SWIipAddress& sa, const void* buf, int len, int msgf)
{
  if (_stmo != -1 && is_writeready (_stmo)==0) return 0;

  int	wlen=0;
  while(len>0)
  {
    int	wval;
    if ((wval = ::sendto (_sd, (char*) buf, len,
			  msgf, sa.addr (), sa.size())) == -1)
    {
      error(L"sendto", 500, WSA_LAST_ERROR);
      return SWIstream::WRITE_ERROR;
    }
    len -= wval;
    wlen += wval;
  }
  return wlen;
}

long SWIsocket::sendtimeout(long wp)
{
  long old_stmo = _stmo;
  _stmo = (wp < 0) ? -1: wp;
  return old_stmo;
}

long SWIsocket::recvtimeout(long wp)
{
  long old_rtmo = _rtmo;
  _rtmo = (wp < 0) ? -1: wp;
  return old_rtmo;
}

int SWIsocket::is_readready(timeval *tv) const
{
  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET (_sd, &rfds);

  fd_set efds;
  FD_ZERO (&efds);
  FD_SET (_sd, &efds);

  int ret = select (_sd + 1, &rfds, 0, &efds, tv);

  if (ret <= 0)
  {
    if (ret < 0) error(L"select", 500, WSA_LAST_ERROR);
    return ret;
  }

  if (FD_ISSET(_sd, &efds))
  {
    int val;
    socklen_t rlen = sizeof val;
    ::getsockopt(_sd, SOL_SOCKET, SO_ERROR, (char *) &val, &rlen);

    error(L"select", 502, val);
    return -1;
  }

  return FD_ISSET(_sd, &rfds);
}

int SWIsocket::is_readready(long timeoutMs) const
{
  timeval tv;
  timeval *ptv = millisecToTimeval(timeoutMs, tv);

  return is_readready(ptv);
}

int SWIsocket::is_writeready(timeval *tv) const
{
  fd_set wfds;
  FD_ZERO (&wfds);
  FD_SET (_sd, &wfds);

  fd_set efds;
  FD_ZERO (&efds);
  FD_SET (_sd, &efds);

  int ret = select (_sd+1, 0, &wfds, &efds, tv);

  if (ret <= 0)
  {
    if (ret < 0) error(L"select", 500, WSA_LAST_ERROR);
    return ret;
  }

  if (FD_ISSET(_sd, &efds))
  {
    int val;
    socklen_t rlen = sizeof val;
    getsockopt(_sd, SOL_SOCKET, SO_ERROR, (char *) &val, &rlen);

    error(L"select", 502, val);
    return -1;
  }

  return FD_ISSET(_sd,&wfds);
}

int SWIsocket::is_writeready(long timeoutMs) const
{
  timeval tv;
  timeval *ptv = millisecToTimeval(timeoutMs, tv);

  return is_writeready(ptv);
}

int SWIsocket::is_exceptionpending(timeval *tv) const
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET  (_sd, &fds);

  int ret = select(_sd+1, 0, 0, &fds, tv);
  if (ret == -1)
  {
    error(L"select", 500);
    return 0;
  }
  return ret;
}

int SWIsocket::is_exceptionpending (long timeoutMs) const
{
  timeval tv;
  timeval *ptv = millisecToTimeval(timeoutMs, tv);

  return is_exceptionpending(ptv);
}

SWIstream::Result SWIsocket::shutdown (shuthow sh)
{
  if (::shutdown(_sd, sh) != 0)
  {
    error(L"shutdown", 500);
    return SWIstream::FAILURE;
  }

  return SWIstream::SUCCESS;
}

int SWIsocket::getopt (option op, void* buf, int len, level l) const
{
  socklen_t rlen = len;
  if (::getsockopt (_sd, l, op, (char*) buf, &rlen) == -1)
    error(L"getsockopt", 500);
  return rlen;
}

void SWIsocket::setopt (option op, void* buf, int len, level l) const
{
  if (::setsockopt (_sd, l, op, (char*) buf, len) == -1)
    error (L"setsockopt", 500);
}

SWIsocket::Type SWIsocket::gettype() const
{
  int	ty=0;
  getopt (so_type, &ty, sizeof (ty));
  return SWIsocket::Type(ty);
}

int SWIsocket::clearerror () const
{
  int 	err=0;
  getopt (so_error, &err, sizeof (err));
  return err;
}

int SWIsocket::debug (int opt) const
{
  int old=0;
  getopt (so_debug, &old, sizeof (old));
  if (opt != -1)
    setopt (so_debug, &opt, sizeof (opt));
  return old;
}

int SWIsocket::reuseaddr (int opt) const
{
  int old=0;
  getopt (so_reuseaddr, &old, sizeof (old));
  if (opt != -1)
    setopt (so_reuseaddr, &opt, sizeof (opt));
  return old;
}

int SWIsocket::keepalive (int opt) const
{
  int old=0;
  getopt (so_keepalive, &old, sizeof (old));
  if (opt != -1)
    setopt (so_keepalive, &opt, sizeof (opt));
  return old;
}

int SWIsocket::dontroute (int opt) const
{
  int old=0;
  getopt (so_dontroute, &old, sizeof (old));
  if (opt != -1)
    setopt (so_dontroute, &opt, sizeof (opt));
  return old;
}

int SWIsocket::broadcast (int opt) const
{
  int old=0;
  getopt (so_broadcast, &old, sizeof (old));
  if (opt != -1)
    setopt (so_broadcast, &opt, sizeof (opt));
  return old;
}

int SWIsocket::oobinline (int opt) const
{
  int old=0;
  getopt (so_oobinline, &old, sizeof (old));
  if (opt != -1)
    setopt (so_oobinline, &opt, sizeof (opt));
  return old;
}

int SWIsocket::linger (int opt) const
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

int SWIsocket::sendbufsz (int  sz) const
{
  int old=0;
  getopt (so_sndbuf, &old, sizeof (old));
  if (sz >= 0)
    setopt (so_sndbuf, &sz, sizeof (sz));
  return old;
}

int SWIsocket::recvbufsz (int sz) const
{
  int old=0;
  getopt (so_rcvbuf, &old, sizeof (old));
  if (sz >= 0)
    setopt (so_rcvbuf, &sz, sizeof (sz));
  return old;
}

void SWIsocket::error(const VXIchar* func, VXIunsigned errorId,
                      const VXIchar *errorName, int errorCode) const
{
  if (_logger)
  {
    _logger->Error(errorId, L"%s%s%s%i",
                   L"Func", func,
                   errorName, errorCode);
  }
}

void SWIsocket::error(const VXIchar* func, VXIunsigned errorId, int errorCode) const
{
  if (_logger)
  {
    _logger->Error(errorId, L"%s%s%s%i",
                   L"Func",
                   func,
                   WSA_ERROR_NAME, errorCode);
  }
}

void SWIsocket::error(const VXIchar* func, VXIunsigned errorId) const
{
  if (_logger)
  {
    _logger->Error(errorId, L"%s%s", L"Func", func);
  }
}
