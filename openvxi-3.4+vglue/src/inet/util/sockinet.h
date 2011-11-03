// sockinet.h -*- C++ -*- socket library
// Copyright (C) 1992,1993,1994 Gnanasekaran Swaminathan <gs4t@virginia.edu>
//
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided the above copyright
// notice and this paragraph are preserved on all copies.
// This software is provided "as is" with no express or implied warranty.
//
// Version: 17Oct95 1.10

#ifndef _SOCKINET_H
#define	_SOCKINET_H

#include "sockstream.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#define ALLOCATE_STREAMBUF

class SWIUTIL_API_CLASS sockinetaddr: public sockAddr, public sockaddr_in {
protected:
  int setport (const char* sn, const char* pn="tcp");
  int setaddr (const char* hn);
  void herror(const char*) const;

public:
  virtual ~sockinetaddr () {}
  sockinetaddr (const sockinetaddr &);
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase);
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase, unsigned long addr, int port_no=0);
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase, const char* host_name, int port_no=0);
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase, unsigned long addr,
                const char* service_name, const char* protocol_name="tcp");
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase, const char* host_name,
                const char* service_name, const char* protocol_name="tcp");
  sockinetaddr (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		VXIunsigned diagTagBase, const sockinetaddr& sina);
  operator void* () const { 
    return (void *)(static_cast<const sockaddr_in*>(this)); }

  socklen_t size() const { return sizeof (sockaddr_in); }
  int family() const { return sin_family; }
  sockaddr* addr() const {
    return (sockaddr*)(static_cast<const sockaddr_in*>(this));}

  int getport() const;
  int gethostname(char *hostname, size_t hostnameLen) const;

private:
  // Disable the copy constructor and assignment operator
  sockinetaddr & operator=(const sockinetaddr &);
};

class SWIUTIL_API_CLASS sockinetbuf: public sockbuf
{
 public:
  sockinetbuf& operator=(const sockbuf& si); // needs to be fixed
  sockinetbuf& operator=(const sockinetbuf& si); // needs fixing

public:
  enum domain { af_inet = AF_INET };
  sockinetbuf (const sockbuf& si): sockbuf(si)
  {}
  sockinetbuf (const sockinetbuf& si): sockbuf (si)
  {}
  sockinetbuf (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	       VXIunsigned diagTagBase, sockbuf::type ty, int proto=0);

  sockbuf* open (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
		 VXIunsigned diagTagBase, sockbuf::type, int proto=0);

  sockinetaddr	localaddr() const;
  int localport() const;
  int localhost(char *hostname, size_t hostnameLen) const;

  sockinetaddr peeraddr() const;
  int peerport() const;
  int peerhost(char *hostname, size_t hostnameLen) const;

  int bind_until_success (int portno);

  virtual int bind (sockAddr& sa);
  int bind ();
  int bind (unsigned long addr, int port_no=0);
  int bind (const char* host_name, int port_no=0);
  int bind (unsigned long addr,
            const char* service_name,
            const char* protocol_name="tcp");
  int bind (const char* host_name,
            const char* service_name,
            const char* protocol_name="tcp");

  virtual int connect (sockAddr& sa);
  int connect (unsigned long addr, int port_no=0);
  int connect (const char* host_name, int port_no=0);
  int connect (unsigned long addr,
               const char* service_name,
               const char* protocol_name="tcp");
  int connect (const char* host_name,
               const char* service_name,
               const char* protocol_name="tcp");
};

class SWIUTIL_API_CLASS isockinet: public isockstream
{
 public:
  isockinet (const sockbuf& sb);
  isockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	     VXIunsigned diagTagBase,
             sockbuf::type ty=sockbuf::sock_stream,
             int proto=0);
  virtual ~isockinet ();

  sockinetbuf*	rdbuf () { return static_cast<sockinetbuf*>(ios::rdbuf ()); }
  sockinetbuf*	operator -> () { return rdbuf (); }

#ifndef ALLOCATE_STREAMBUF
 private:
  sockinetbuf _mysb;
#endif

private:
  // Disable the copy constructor and assignment operator
  isockinet (const isockinet &);
  isockinet & operator=(const isockinet &);
};

class SWIUTIL_API_CLASS osockinet: public osockstream
{
 public:
  osockinet (const sockbuf& sb);
  osockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	     VXIunsigned diagTagBase,
             sockbuf::type ty=sockbuf::sock_stream,
             int proto=0);
  virtual ~osockinet ();

  sockinetbuf*	rdbuf () { return static_cast<sockinetbuf*>(ios::rdbuf ()); }
  sockinetbuf*	operator -> () { return rdbuf (); }

#ifndef ALLOCATE_STREAMBUF
 private:
  sockinetbuf _mysb;
#endif

private:
  // Disable the copy constructor and assignment operator
  osockinet (const osockinet &);
  osockinet & operator=(const osockinet &);
};

class SWIUTIL_API_CLASS iosockinet: public iosockstream
{
 public:
  iosockinet (const sockbuf& sb);
  iosockinet (const VXIchar *moduleName, VXIlogInterface *pVXILog, 
	      VXIunsigned diagTagBase, 
              sockbuf::type ty=sockbuf::sock_stream,
              int proto=0);
  virtual ~iosockinet ();

  sockinetbuf*	rdbuf () { return static_cast<sockinetbuf*>(ios::rdbuf ()); }
  sockinetbuf*	operator -> () { return rdbuf (); }

#ifndef ALLOCATE_STREAMBUF
 private:
  sockinetbuf _mysb;
#endif

private:
  // Disable the copy constructor and assignment operator
  iosockinet (const iosockinet &);
  iosockinet & operator=(const iosockinet &);
};

#endif	// _SOCKINET_H
