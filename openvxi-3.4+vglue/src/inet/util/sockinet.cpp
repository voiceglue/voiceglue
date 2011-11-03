// sockinet.C  -*- C++ -*- socket library
// Copyright (C) 1992,1993,1994 Gnanasekaran Swaminathan <gs4t@virginia.edu>
//
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided the above copyright
// notice and this paragraph are preserved on all copies.
// This software is provided "as is" with no express or implied warranty.
//
// Version: 17Oct95 1.10

#include "SWIutilInternal.h"

#include "sockinet.h"

#ifndef _WIN32
EXTERN_C_BEGIN
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
EXTERN_C_END
#else
#include <winsock.h>
#define EADDRINUSE WSAEADDRINUSE
#endif /* !_WIN32 */

#if defined(WIN32) || defined(_decunix_)
/* These OSes are thread safe by using thread local storage for the hostent */
static int my_gethostbyaddr_r (const char * addr, socklen_t len,
			       int type,
			       struct hostent * result_buf,
			       char * buf, size_t buflen,
			       struct hostent ** result,
			       int * h_errnop) {
  *result = gethostbyaddr(addr, len, type);
  return (result ? 0 : -1);
}

static int my_gethostbyname_r (const char * name,
			       struct hostent * result_buf,
			       char * buf, size_t buflen,
			       struct hostent ** result,
			       int * h_errnop) {
  *result = gethostbyname(name);
  return (result ? 0 : -1);
}
#elif defined(_solaris_)
static int my_gethostbyaddr_r (const char * addr, socklen_t len,
			       int type,
			       struct hostent * result_buf,
			       char * buf, size_t buflen,
			       struct hostent ** result,
			       int * h_errnop) {
  *result = gethostbyaddr_r(addr, len, type, result_buf, buf, buflen,
			    h_errnop);
  return (result ? 0 : -1);
}

static int my_gethostbyname_r (const char * name,
			       struct hostent * result_buf,
			       char * buf, size_t buflen,
			       struct hostent ** result,
			       int * h_errnop) {
  *result = gethostbyname_r(name, result_buf, buf, buflen, h_errnop);
  return (result ? 0 : -1);
}
#else
/* Linux, maybe others */
#define my_gethostbyaddr_r gethostbyaddr_r
#define my_gethostbyname_r gethostbyname_r
#endif

sockinetaddr::sockinetaddr (const sockinetaddr &sina)
  : sockAddr (sina.GetModuleName( ), sina.GetLog( ), sina.GetDiagBase( ))
{
  sin_family      = sina.sin_family;
  sin_addr.s_addr = sina.sin_addr.s_addr;
  sin_port	  = sina.sin_port;
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase)
  : sockAddr (moduleName, pVXILog, diagTagBase)
{
  sin_family	  = sockinetbuf::af_inet;
  sin_addr.s_addr = htonl(INADDR_ANY);
  sin_port	  = 0;
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase, unsigned long addr,
			   int port_no)
  : sockAddr (moduleName, pVXILog, diagTagBase)
// addr and port_no are in host byte order
{
  sin_family      = sockinetbuf::af_inet;
  sin_addr.s_addr = htonl(addr);
  sin_port	  = htons(port_no);
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase, unsigned long addr,
			   const char* sn, const char* pn)
  : sockAddr (moduleName, pVXILog, diagTagBase)
// addr is in host byte order
{
  sin_family      = sockinetbuf::af_inet;
  sin_addr.s_addr = htonl (addr); // Added by cgay@cs.uoregon.edu May 29, 1993
  setport(sn, pn);
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase, const char* host_name,
			   int port_no)
  : sockAddr (moduleName, pVXILog, diagTagBase)
// port_no is in host byte order
{
  setaddr(host_name);
  sin_port = htons(port_no);
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase, const char* hn, 
			   const char* sn, const char* pn)
  : sockAddr (moduleName, pVXILog, diagTagBase)
{
  setaddr(hn);
  setport(sn, pn);
}

sockinetaddr::sockinetaddr(const VXIchar *moduleName, VXIlogInterface *pVXILog,
			   VXIunsigned diagTagBase, const sockinetaddr& sina)
  : sockAddr (moduleName, pVXILog, diagTagBase)
{
  sin_family      = sina.sin_family;
  sin_addr.s_addr = sina.sin_addr.s_addr;
  sin_port	  = sina.sin_port;
}

int sockinetaddr::setport(const char* sn, const char* pn)
{
  servent* sp = getservbyname(sn, pn);
  if (sp == 0)
  {
    SOCK_ERROR("sockinetaddr", "invalid service name");
    return -1;
  }
  sin_port = sp->s_port;
  return 0;
}

int sockinetaddr::getport () const
{
  return ntohs (sin_port);
}

int sockinetaddr::setaddr(const char* host_name)
{
  if ( (sin_addr.s_addr = inet_addr(host_name)) == (in_addr_t) -1) {
    int hostErrno;
    struct hostent hostResult;
    char hostBuffer[4096];
    hostent* hp;
    if ((my_gethostbyname_r(host_name, &hostResult, hostBuffer,
			    2048, &hp, &hostErrno) != 0) ||
	(hp == 0))
    {
      return -1;
    }
    memcpy(&sin_addr, hp->h_addr, hp->h_length);
    sin_family = hp->h_addrtype;
  }
  else
    sin_family = sockinetbuf::af_inet;
  return 0;
}

int sockinetaddr::gethostname (char *hostname, size_t hostnameLen) const
{
  hostname[0] = '\0';

  if (sin_addr.s_addr == htonl(INADDR_ANY)) {
    if (::gethostname(hostname, hostnameLen) == -1) {
      SOCK_ERROR("sockinetaddr", "gethostname");
      hostname[0] = '\0';
      return -1;
    }
    return 0;
  }

  int hostErrno;
  struct hostent hostResult;
  char hostBuffer[4096];
  hostent* hp;
  if ((my_gethostbyaddr_r((const char*) &sin_addr, sizeof(sin_addr),
			  family(), &hostResult, hostBuffer, 2048, &hp,
			  &hostErrno) != 0) ||
      (hp == 0))
  {
    sockinetaddr::herror("gethostbyaddr");
    return -1;
  }
  if ((hp->h_name) && (strlen(hp->h_name) < hostnameLen)) {
    strcpy (hostname, hp->h_name);
    return 0;
  }
  return -1;
}

sockinetbuf::sockinetbuf(const VXIchar *moduleName, VXIlogInterface *pVXILog, 
			 VXIunsigned diagTagBase, 
			 sockbuf::type ty, int proto)
  : sockbuf(moduleName, pVXILog, diagTagBase, af_inet, ty, proto)
{}

sockinetbuf& sockinetbuf::operator = (const sockbuf& si)
{
  this->sockbuf::operator = (si);
  return *this;
}

sockinetbuf& sockinetbuf::operator = (const sockinetbuf& si)
{
  this->sockbuf::operator = (si);
  return *this;
}

sockbuf* sockinetbuf::open(const VXIchar *moduleName, 
			   VXIlogInterface *pVXILog, 
			   VXIunsigned diagTagBase,
			   sockbuf::type st, int proto)
{
  *this = sockinetbuf(moduleName, pVXILog, diagTagBase, st, proto);
  return this;
}

sockinetaddr sockinetbuf::localaddr() const
{
  sockinetaddr sin (GetModuleName(), GetLog(), GetDiagBase());
  socklen_t len = sin.size();
  if (::getsockname(rep->sock, sin.addr (), &len) == -1)
    SOCK_ERROR("sockinetbuf", "getsockname");
  return sin;
}

int sockinetbuf::localport() const
{
  sockinetaddr sin (localaddr());
  if (sin.family() != af_inet) return -1;
  return sin.getport();
}

int sockinetbuf::localhost(char *hostname, size_t hostnameLen) const
{
  hostname[0] = '\0';
  sockinetaddr sin (localaddr());
  if (sin.family() != af_inet) return -1;
  return sin.gethostname(hostname, hostnameLen);
}


sockinetaddr sockinetbuf::peeraddr() const
{
  sockinetaddr sin (GetModuleName(), GetLog(), GetDiagBase());
  socklen_t len = sin.size();
  if (::getpeername(rep->sock, sin.addr (), &len) == -1)
    SOCK_ERROR("sockinetbuf", "getpeername");
  return sin;
}

int sockinetbuf::peerport() const
{
  sockinetaddr sin = peeraddr();
  if (sin.family() != af_inet) return -1;
  return sin.getport();
}

int sockinetbuf::peerhost(char *hostname, size_t hostnameLen) const
{
  hostname[0] = '\0';
  sockinetaddr sin = peeraddr();
  if (sin.family() != af_inet) return -1;
  return sin.gethostname(hostname, hostnameLen);
}

int sockinetbuf::bind_until_success (int portno)
// a. bind to (INADDR_ANY, portno)
// b. if success return 0
// c. if failure and errno is EADDRINUSE, portno++ and go to step a.
// d. return errno.
{
  for (;;) {
    sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), 
		     (unsigned long) INADDR_ANY, portno++);
    int eno = bind (sa);
    if (eno == 0)
      return 0;
    if (errno != EADDRINUSE)
      return errno;
  }
}

int sockinetbuf::bind (sockAddr& sa)
{
  return sockbuf::bind (sa);
}

int sockinetbuf::bind ()
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase());
  return bind (sa);
}

int sockinetbuf::bind (unsigned long addr, int port_no)
// address and portno are in host byte order
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), addr, port_no);
  return bind (sa);
}

int sockinetbuf::bind (const char* host_name, int port_no)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), host_name,
		   port_no);
  return bind (sa);
}

int sockinetbuf::bind (unsigned long addr,
		       const char* service_name,
		       const char* protocol_name)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), addr,
		   service_name, protocol_name);
  return bind (sa);
}

int sockinetbuf::bind (const char* host_name,
		       const char* service_name,
		       const char* protocol_name)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), host_name,
		   service_name, protocol_name);
  return bind (sa);
}

int sockinetbuf::connect (sockAddr& sa)
{
  return sockbuf::connect (sa);
}

int sockinetbuf::connect (unsigned long addr, int port_no)
// address and portno are in host byte order
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), addr, port_no);
  return connect (sa);
}

int sockinetbuf::connect (const char* host_name, int port_no)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), host_name,
		   port_no);
  return connect (sa);
}

int sockinetbuf::connect (unsigned long addr,
			  const char* service_name,
			  const char* protocol_name)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), addr,
		   service_name, protocol_name);
  return connect (sa);
}

int sockinetbuf::connect (const char* host_name,
			  const char* service_name,
			  const char* protocol_name)
{
  sockinetaddr sa (GetModuleName(), GetLog(), GetDiagBase(), host_name,
		   service_name, protocol_name);
  return connect (sa);
}

isockinet::isockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		      VXIunsigned diagTagBase,
                      sockbuf::type ty, int proto)
#ifdef ALLOCATE_STREAMBUF
  : isockstream (new sockinetbuf (moduleName, pVXILog, diagTagBase, ty,
				  proto))
#else
  : isockstream (&_mysb), _mysb (moduleName, pVXILog, diagTagBase, ty, proto)
#endif
{}

isockinet::isockinet (const sockbuf& sb)
#ifdef ALLOCATE_STREAMBUF
  : isockstream (new sockinetbuf (sb))
#else
  : isockstream (&_mysb), _mysb(sb)
#endif
{}

isockinet::~isockinet ()
{
#ifdef ALLOCATE_STREAMBUF
  delete ios::rdbuf ();
#endif
#ifdef INIT_IN_DESTRUCTOR
  init (0);
#endif
}

osockinet::osockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		      VXIunsigned diagTagBase,
                      sockbuf::type ty, int proto)
#ifdef ALLOCATE_STREAMBUF
  : osockstream (new sockinetbuf (moduleName, pVXILog, diagTagBase, ty,
				  proto))
#else
  : osockstream (&_mysb), _mysb (moduleName, pVXILog, diagTagBase, ty, proto)
#endif
{}

osockinet::osockinet (const sockbuf& sb)
#ifdef ALLOCATE_STREAMBUF
  : osockstream(new sockinetbuf (sb))
#else
  : osockstream(&_mysb), _mysb (sb)
#endif
{}

osockinet::~osockinet ()
{
#ifdef ALLOCATE_STREAMBUF
  delete ios::rdbuf ();
#endif
#ifdef INIT_IN_DESTRUCTOR
  init (0);
#endif
}

iosockinet::iosockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
			VXIunsigned diagTagBase, 
                        sockbuf::type ty, int proto)
#ifdef ALLOCATE_STREAMBUF
  : iosockstream(new sockinetbuf (moduleName, pVXILog, diagTagBase, ty, proto))
#else
  : iosockstream(&_mysb), _mysb (moduleName, pVXILog, diagTagBase, ty, proto)
#endif
{}

iosockinet::iosockinet (const sockbuf& sb)
#ifdef ALLOCATE_STREAMBUF
  : iosockstream(new sockinetbuf (sb))
#else
  : iosockstream(&_mysb), _mysb (sb)
#endif
{}

iosockinet::~iosockinet ()
{
#ifdef ALLOCATE_STREAMBUF
  delete ios::rdbuf ();
#endif
#ifdef INIT_IN_DESTRUCTOR
  init (0);
#endif
}

#ifdef _WIN32

static	char* errmsg[] = {
  "No error",
  "Host not found",
  "Try again",
  "No recovery",
  "No address"
  "Unknown error"
};

/* Winsock.h maps h_errno to WSAGetLastError() function call */
void sockinetaddr::herror(const char* em) const
{
  int err;
  switch(h_errno) { /* calls WSAGetLastError() */
   case HOST_NOT_FOUND:
     err = 1;
     break;
   case TRY_AGAIN:
     err = 2;
     break;
   case NO_RECOVERY:
     err = 3;
     break;
   case NO_ADDRESS:
     err = 4;
     break;
   default:
     err = 5;
     break;
  }

  SOCK_ERROR(em, errmsg[err]);
}

#else // !_WIN32

/* Use the native UNIX call */
void sockinetaddr::herror(const char* em) const
{
#ifdef _decunix_
  if (errno)
    Error(500, L"%s%S%s%d%s%S%s%d", L"class", em, L"h_errno", h_errno,
	  L"errnoStr", strerror(errno), L"errno", errno);
  else
    Error(500, L"%s%S%s%d", L"class", em, L"h_errno", h_errno);
#else
  SOCK_ERROR(em, hstrerror(h_errno));
#endif
}

#endif // !_WIN32
