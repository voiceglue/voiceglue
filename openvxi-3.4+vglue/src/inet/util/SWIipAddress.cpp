
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

#include <string.h>
#include "SWIipAddress.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#endif

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

#ifdef _WIN32
// Class used for static initialization of mutex or WSA layer.
class SWIipAddressInit
{
 public:
  SWIipAddressInit();
  ~SWIipAddressInit();
};

SWIipAddressInit::SWIipAddressInit()
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SWIipAddressInit::~SWIipAddressInit()
{
  WSACleanup();
}

static SWIipAddressInit G_netInit;

#endif

#define SOCK_ERROR(classname, msg) \
do { \
  if (_logger) \
  { \
    if (errno) \
      _logger->Error(500, L"%s%S%s%S%s%S%s%d", L"class", classname, L"msg", msg, \
	  L"errnoStr", strerror(errno), L"errno", errno); \
    else \
      _logger->Error(500, L"%s%S%s%S", L"class", classname, L"msg", msg); \
  } \
} while(0)


SWIipAddress::SWIipAddress(const SWIipAddress &sina)
{
  _logger         = sina._logger;
  sin_family      = sina.sin_family;
  sin_addr.s_addr = sina.sin_addr.s_addr;
  sin_port	  = sina.sin_port;
}

SWIipAddress::SWIipAddress(SWIutilLogger *logger)
{
  _logger =       logger;
  sin_family	  = AF_INET;
  sin_addr.s_addr = htonl(INADDR_ANY);
  sin_port	  = 0;
}

SWIipAddress::SWIipAddress(unsigned long addr,
			   int port_no,
                           SWIutilLogger *logger)
{
// addr and port_no are in host byte order
  _logger         = logger;
  sin_family      = AF_INET;
  sin_addr.s_addr = htonl(addr);
  sin_port	  = htons(port_no);
}

SWIipAddress::SWIipAddress(unsigned long addr,
			   const char* sn,
                           const char* pn,
                           SWIutilLogger *logger)
{
  // addr is in host byte order
  _logger         = logger;
  sin_family      = AF_INET;
  sin_addr.s_addr = htonl (addr);
  setport(sn, pn);
}

SWIipAddress::SWIipAddress(const char* host_name,
                           int port_no,
                           SWIutilLogger *logger)
{
// port_no is in host byte order
  _logger = logger;
  sethostname(host_name);
  sin_port = htons(port_no);
}

SWIipAddress::SWIipAddress(const char* hn,
			   const char* sn,
                           const char* pn,
                           SWIutilLogger *logger)
{
  _logger = logger;
  sethostname(hn);
  setport(sn, pn);
}

void SWIipAddress::setport(int port_no)
{
  sin_port = htons(port_no);
}

int SWIipAddress::setport(const char* sn, const char* pn)
{
  servent* sp = getservbyname(sn, pn);
  if (sp == 0)
  {
    SOCK_ERROR("SWIipAddress", "invalid service name");
    return -1;
  }
  sin_port = sp->s_port;
  return 0;
}

int SWIipAddress::getport() const
{
  return ntohs (sin_port);
}

int SWIipAddress::sethostname(const char* host_name)
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
    sin_family = AF_INET;
  return 0;
}

int SWIipAddress::gethostname(char *hostname, size_t hostnameLen) const
{
  hostname[0] = '\0';

  if (sin_addr.s_addr == htonl(INADDR_ANY)) {
    if (::gethostname(hostname, hostnameLen) == -1) {
      SOCK_ERROR("SWIipAddress", "gethostname");
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
    SWIipAddress::herror("gethostbyaddr");
    return -1;
  }
  if ((hp->h_name) && (strlen(hp->h_name) < hostnameLen)) {
    strcpy (hostname, hp->h_name);
    return 0;
  }
  return -1;
}


unsigned int SWIipAddress::hashCode() const
{
  unsigned int h = (unsigned int) ntohl(sin_addr.s_addr);
  h <<= 16;
  h |= ntohs(sin_port) & 0xFF;
  return h;
}

bool SWIipAddress::operator==(const SWIipAddress& rhs) const
{
  if (this == &rhs) return true;

  return sin_addr.s_addr == rhs.sin_addr.s_addr &&
    sin_port == rhs.sin_port;
}

SWIHashable *SWIipAddress::clone() const
{
  return new SWIipAddress(*this);
}

bool SWIipAddress::equals(const SWIHashable *rhs) const
{
  return operator==(*((const SWIipAddress *) rhs));
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
void SWIipAddress::herror(const char* em) const
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
void SWIipAddress::herror(const char* em) const
{
#ifdef _decunix_
  if (_logger)
  {
    if (errno)
      Error(500, L"%s%S%s%d%s%S%s%d", L"class", em, L"h_errno", h_errno,
            L"errnoStr", strerror(errno), L"errno", errno);
    else
      Error(500, L"%s%S%s%d", L"class", em, L"h_errno", h_errno);
  }
#else
  SOCK_ERROR(em, hstrerror(h_errno));
#endif
}

#endif // !_WIN32

#if 0
SWIipAddress::SWIipAddress (int port)
   : ipAddressSet(false)
{
  char *localHost = "localhost";
  sethostname(localHost); // assuming it is localhost first
  aPort = port;
}


SWIipAddress::SWIipAddress ( const char * hostName, int port)
  : ipAddressSet(false)
{
  setport(port);
  setHostName(hostName);
}


int SWIipAddress::setHostName ( const char * theAddress)
{
  //
  // Look for semicolon which means the port address is appended
  //
  int colonPos = -1;

  const char * cstrAddress = theAddress;

  while (*cstrAddress)
  {
    if (*cstrAddress == ':')
    {
      colonPos = cstrAddress - theAddress;
      break;
    }
    cstrAddress++;
  }

  //
  // Found the port address so get it
  //
  if (colonPos != -1)
  {
    aPort = atoi(theAddress + (colonPos + 1));

    if (aPort)
    {
      strncpy(rawHostName, theAddress, colonPos - 1);
      rawHostName[colonPos] = '\0';
    }
    else
    {
      return -1;
    }

  }
  else // No ':' in input string;
  {
    strcpy(rawHostName,theAddress);
  }

  ipAddressSet = false;
  return 0;
}


void SWIipAddress::setport( int iPort )
{
    ipAddressSet = false;
    aPort = iPort;
}


char * SWIipAddress::getHostName( ) const
{
  char * hostName;
  struct hostent * pxHostEnt = NULL;

  initIpAddress();

  if ((my_gethostbyaddr_r(
  {

  }
  LOCK();

  pxHostEnt = gethostbyaddr(ipAddress, IPV4_LENGTH, AF_INET);

  if (pxHostEnt)
  {
    hostName = pxHostEnt->h_name;
    UNLOCK();
  }
  else
  {
    UNLOCK();
    hostName = getIpName();
  }

  return hostName;
}


char * SWIipAddress::getIpName () const
{
    char * ipName;
    struct in_addr temp;

    initIpAddress();

    memcpy((void *)&temp.s_addr, ipAddress, IPV4_LENGTH);

    LOCK();

    // $$$ This returns a pointer to a static char[]
    ipName = inet_ntoa(temp);

    UNLOCK();

    return ipName;
}


unsigned int SWIipAddress::getIp4Address () const
{
    unsigned lTmp;

    initIpAddress();

    memcpy((void *) &lTmp, ipAddress, IPV4_LENGTH);

    return lTmp;
}


void SWIipAddress::getSockAddr (struct sockaddr & socka) const
{
    short tmp_s = htons (aPort);
    char * tmp_p;

    tmp_p = (char*) &tmp_s;
    socka.sa_family = AF_INET;
    socka.sa_data[0] = tmp_p[0];
    socka.sa_data[1] = tmp_p[1];
    initIpAddress();
    memcpy((void *)&socka.sa_data[2], ipAddress, IPV4_LENGTH);

    return ;
}


int SWIipAddress::getPort () const
{
    return aPort;
}


bool operator < ( const SWIipAddress & xAddress, const SWIipAddress & yAddress )
{
    xAddress.initIpAddress();
    yAddress.initIpAddress();
    for (int i = 0 ; i <= IPV4_LENGTH; i++ )
    {
        if ( xAddress.ipAddress[i] < yAddress.ipAddress[i])
        {
            return true;
        }
        if ( xAddress.ipAddress[i] > yAddress.ipAddress[i])
        {
            return false;
        }
    }
    return xAddress.aPort < yAddress.aPort ;
}


bool operator == ( const SWIipAddress & xAddress, const SWIipAddress & yAddress )
{
    xAddress.initIpAddress();
    yAddress.initIpAddress();
    for (int i = 0 ; i < IPV4_LENGTH ; i++ )
    {
        if ( xAddress.ipAddress[i] != yAddress.ipAddress[i])
        {
            return false;
        }
    }
    return xAddress.aPort == yAddress.aPort ;
}


bool operator != ( const SWIipAddress & xAddress, const SWIipAddress & yAddress )
{
    return !(xAddress == yAddress);
}


SWIipAddress & SWIipAddress::operator=( const SWIipAddress& x )
{
    this->aPort = x.aPort;
    memcpy( this->ipAddress, x.ipAddress, sizeof(this->ipAddress) );
    ipAddressSet = x.ipAddressSet;

    memcpy(this->rawHostName, x.rawHostName, sizeof(this->rawHostName));
    // xraf rawHostName = x.rawHostName;
    //   strncpy( this->ipAddress, x.ipAddress, IPV4_LENGTH );
    return ( *this );
}


unsigned int SWIipAddress::hashIpPort( )
{
    unsigned int ipAddressLocal = getIp4Address ();
    int port = getPort ();
    return (hashIpPort( ipAddressLocal, port )) ;
}


unsigned int SWIipAddress::hashIpPort( const char * theAddress, const char * port )
{
    SWIipAddress net;

    net.setHostName ( theAddress );
    net.setport ( atoi(port) );

    return ( net.hashIpPort() );
}


unsigned int SWIipAddress::hashIpPort( const unsigned int ipAddress, const int port )
{
    unsigned int hashKey = 0x0;
    hashKey = (ipAddress << 16) | (port & 0xFF);

    return hashKey;
}

int SWIipAddress::initIpAddress() const
{
  int status = 0;

  if (!ipAddressSet)
  {
    if (is_valid_ip_addr(rawHostName) )
    {
      ipAddressSet = true;
    }
    else
    {
      LOCK();
      struct hostent * pxHostEnt = 0;
      pxHostEnt = gethostbyname(rawHostName);

      if (pxHostEnt)
      {
        memcpy((void *)&ipAddress,
               (void *)pxHostEnt->h_addr,
               pxHostEnt->h_length);
        ipAddressSet = true;
      }
      else
      {
        status = -1;
      }

      UNLOCK();
    }
  }
  return status;
}


/**
  * Return TRUE if the address is a valid IP address (dot-decimal form)
  * such as 128.128.128.128. Checks for invalid or ill formed addresses
  * If the address is valid, copy it into ipAddress
  */
bool SWIipAddress::is_valid_ip_addr(const char * addr) const
{

    unsigned long maskcheck = ~255; // mask to check if 'tween 0 and 255
    const char *advancer = addr;
    unsigned long octet;
    char *nextchar;

    // always check for spaces and right number
    // first and last fields must be 1-255, middle two 0-255

    if ((*(advancer) == 0) || (*(advancer) == ' ') || (*(advancer) == '\t'))
    {
        return false;
    }
    octet = strtoul(advancer, &nextchar, 10);
    if((*nextchar != '.') || (octet & maskcheck) || (octet == 0))
    {
        return false;
    }
    ipAddress[0] = (char) octet;


    advancer = nextchar+1;
    if ((*(advancer) == 0) || (*(advancer) == ' ') || (*(advancer) == '\t'))
    {
        return false;
    }
    octet = strtoul(advancer, &nextchar, 10);
    if((*nextchar != '.') || (octet & maskcheck))
    {
        return false;
    }
    ipAddress[1] = (char) octet;


    advancer = nextchar+1;
    if ((*(advancer) == 0) || (*(advancer) == ' ') || (*(advancer) == '\t'))
    {
        return false;
    }
    octet = strtoul(advancer, &nextchar, 10);
    if((*nextchar != '.') || (octet & maskcheck))
    {
        return false;
    }
    ipAddress[2] = (char) octet;


    advancer = nextchar+1;
    if ((*(advancer) == 0) || (*(advancer) == ' ') || (*(advancer) == '\t'))
    {
        return false;
    }
    octet = strtoul(advancer, &nextchar, 10);
    if((*nextchar) || (octet & maskcheck) || (octet == 0))
    {
        return false;
    }
    ipAddress[3] = (char) octet;

    return true;

}
#endif
// End of File
