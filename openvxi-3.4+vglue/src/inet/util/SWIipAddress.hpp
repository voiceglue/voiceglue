#ifndef SWIIPADDRESS_HPP
#define SWIIPADDRESS_HPP

#include "SWIutilHeaderPrefix.h"

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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#ifdef _solaris_
#include <sys/socket.h>
#endif
#endif

#ifdef _WIN32
typedef int socklen_t;
typedef unsigned long in_addr_t;
#else
#if defined(_linux_) && (! defined(IPPROTO_IP))
typedef uint32_t in_addr_t;
#endif
#endif

#include "SWIutilLogger.hpp"
#include "SWIHashable.hpp"

/** Class which implements a network address.  includes functionality
 * to look up network addresses from hostnames or ip addresses as
 * strings.
 */

class SWIUTIL_API_CLASS SWIipAddress : public sockaddr_in, public SWIHashable
{
 public:
  SWIipAddress (const SWIipAddress &);
  SWIipAddress (SWIutilLogger *logger = NULL);

  SWIipAddress (unsigned long addr,
                int port_no=0,
                SWIutilLogger *logger = NULL);

  SWIipAddress (const char* host_name,
                int port_no=0,
                SWIutilLogger *logger = NULL);

  SWIipAddress (unsigned long addr,
                const char* service_name,
                const char* protocol_name="tcp",
                SWIutilLogger *logger = NULL);

  SWIipAddress (const char* host_name,
                const char* service_name, const char* protocol_name="tcp",
                SWIutilLogger *logger = NULL);

  virtual ~SWIipAddress()
  {}

  virtual operator const void*() const
  {
    return (void *)(static_cast<const sockaddr_in*>(this));
  }

  virtual operator void*()
  {
    return (void *)(static_cast<const sockaddr_in*>(this));
  }

  operator const struct sockaddr*() const { return addr (); }
  operator struct sockaddr*() { return addr (); }

  virtual socklen_t size() const { return sizeof (sockaddr_in); }
  virtual int family() const { return sin_family; }

  virtual struct sockaddr* addr()
  {
    return (sockaddr*)(static_cast<sockaddr_in*>(this));
  }

  virtual const struct sockaddr* addr() const
  {
    return (const sockaddr*)(static_cast<const sockaddr_in*>(this));
  }

  int getport() const;
  int gethostname(char *hostname, size_t hostnameLen) const;

 public:
  void setport(int port_no);

 public:
  virtual unsigned int hashCode() const;
  virtual bool equals(const SWIHashable *rhs) const;
  virtual SWIHashable* clone() const;

  bool operator ==(const SWIipAddress& rhs) const;

  bool operator !=(const SWIipAddress& rhs) const
  {
    return !operator==(rhs);
  }



 public:
  int setport(const char* sn, const char* pn="tcp");
  int sethostname(const char* hn);
 protected:
  void herror(const char*) const;

 private:
  SWIutilLogger *_logger;
};

#endif
