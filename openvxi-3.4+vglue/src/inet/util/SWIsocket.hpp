#ifndef SWISOCKET_HPP
#define SWISOCKET_HPP

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

#include "SWIutilHeaderPrefix.h"
#include "SWIinputStream.hpp"
#include "SWIoutputStream.hpp"
#include "SWIutilLogger.hpp"
#include "VXIlog.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#endif

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

class SWIipAddress;
class SWIoutputStream;

/**
 * This class implements client and server sockets.  A server socket is
 * listening and accepting connections from client sockets.
 */
class SWIUTIL_API_CLASS SWIsocket
{
 public:
  enum Type
  {
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

 public:

  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Creates an unconnected Socket.
   * <P>
   * @param socktype The type of socket.
   * Use sock_stream for TCP and sock_dgram for UDP.
   *
   */
 public:
  SWIsocket(Type socktype = sock_stream,
            SWIutilLogger *logger = NULL);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIsocket();

  /**
   * Connects this socket to the server specified in the ip address object.
   *
   */
 public:
  virtual SWIstream::Result connect(const SWIipAddress& endpoint, long timeout = -1);
  SWIstream::Result connect(const char *host, int port = 0, long timeout = -1);
  SWIstream::Result connect(unsigned long addr, int port = 0, long timeout = -1);
  SWIstream::Result connect(unsigned long addr, const char *service_name,
                 const char *protocol_name = "tcp", long timeout = -1);
  SWIstream::Result connect(const char *host, const char *service_name,
                 const char *protocol_name = "tcp", long timeout = -1);

  virtual SWIstream::Result bind(const SWIipAddress &addr);
  SWIstream::Result bind();
  SWIstream::Result bind(unsigned long addr, int port_no=0);
  SWIstream::Result bind(const char* host_name, int port_no=0);
  SWIstream::Result bind(unsigned long addr,
              const char* service_name,
              const char* protocol_name="tcp");
  SWIstream::Result bind(const char* host_name,
              const char* service_name,
              const char* protocol_name="tcp");


  SWIstream::Result listen(int backlog=somaxconn);

  virtual SWIsocket *accept(SWIipAddress& sa);
  virtual SWIsocket *accept();

  /**
   * Returns the address to which the socket is connected.
   *
   * @return  the remote IP address to which this socket is connected,
   *		or <code>null</code> if the socket is not connected.
   */
 public:
  const SWIipAddress* getRemoteAddress() const;

  /**
   * Gets the local address to which the socket is bound.
   *
   * @return the local address to which the socket is bound or NULL
   *	       if the socket is not bound yet.
   */
 public:
  const SWIipAddress* getLocalAddress() const;

  /**
   * Returns the remote port to which this socket is connected.
   *
   * @return  the remote port number to which this socket is connected, or
   *	        0 if the socket is not connected yet.
   */
 public:
  int getRemotePort() const;

  /**
   * Returns the local port to which this socket is bound.
   *
   * @return  the local port number to which this socket is bound or -1
   *	        if the socket is not bound yet.
   */
 public:
  int getLocalPort() const;

  /**
   * Returns an input stream for this socket.  The input stream must be
   * deleted by the caller.  If an error occurs, returns NULL.  There is one
   * input stream per socket.  Calling this function several times will return
   * the same stream, so care must be taken to ensure that it is deleted only
   * once.  The stream can be closed either by calling its close method or by
   * shutting down the read end of the socket.
   */
 public:
  SWIinputStream* getInputStream();

  /**
   * Returns an output stream for this socket.  The output stream must be
   * deleted by the caller.  If an error occurs, returns NULL.  There is one
   * output stream per socket.  Calling this function several times will return
   * the same stream, so care must be taken to ensure that it is deleted only
   * once.  The stream can be closed either by calling its close method or by
   * shutting down the write end of the socket.
   */
 public:
  SWIoutputStream* getOutputStream();

  virtual int read(void* buf, int len);
  virtual int recv(void* buf, int len, int msgf=0);
  virtual int recvfrom(SWIipAddress& sa, void* buf, int len, int msgf=0);

  virtual int write	(const void* buf, int len);
  virtual int send	(const void* buf, int len, int msgf=0);
  virtual int sendto	(const SWIipAddress& sa, const void* buf, int len, int msgf=0);

  long sendtimeout (long timeoutMs=-1);
  long recvtimeout (long timeoutMs=-1);
  int is_readready (long timeoutMs) const;
  int is_writeready (long timeoutMs) const;
  int is_exceptionpending (long timeoutMs) const;
  int is_readready (timeval *tv = NULL) const;
  int is_writeready (timeval *tv = NULL) const;
  int is_exceptionpending (timeval *tv = NULL) const;

  virtual SWIstream::Result shutdown (shuthow sh);
  int getopt(option op, void* buf,int len,
             level l=sol_socket) const;

  void setopt(option op, void* buf,int len,
              level l=sol_socket) const;

  Type gettype () const;

  int clearerror () const;
  int debug	  (int opt= -1) const;
  int reuseaddr (int opt= -1) const;
  int keepalive (int opt= -1) const;
  int dontroute (int opt= -1) const;
  int broadcast (int opt= -1) const;
  int oobinline (int opt= -1) const;
  int linger    (int tim= -1) const;
  int sendbufsz (int sz=-1)   const;
  int recvbufsz (int sz=-1)   const;

  /**
   * Closes this socket.
   */
 public:
  virtual SWIstream::Result close();

 protected:
  void error (const VXIchar *func, VXIunsigned errorId, int errorCode) const;
  void error (const VXIchar *func, VXIunsigned errorId,
              const VXIchar *errorName, int errorCode) const;
  void error (const VXIchar *func, VXIunsigned errorId) const;

 private:
  class SWIUTIL_API_CLASS InputStream: public SWIinputStream
  {
   public:
    InputStream(SWIsocket *sock);
    virtual ~InputStream();

    virtual int readBytes(void * data, int dataSize);
    virtual SWIstream::Result close();
    virtual SWIstream::Result waitReady(long timeoutMs = -1);

   private:
    friend class SWIsocket;
    SWIsocket *_sock;
  };

 private:
  class SWIUTIL_API_CLASS OutputStream: public SWIoutputStream
  {
   public:
    OutputStream(SWIsocket *sock);
    ~OutputStream();

    virtual int writeBytes(const void *buffer, int bufferSize);
    virtual SWIstream::Result close();
    virtual SWIstream::Result waitReady(long timeoutMs = -1);

   private:
    friend class SWIsocket;
    SWIsocket *_sock;
  };

  /**
   * Disabled copy constructor.
   **/
 private:
  SWIsocket(const SWIsocket&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SWIsocket& operator=(const SWIsocket&);

 private:
  SWIsocket(const SWIsocket *socket, SOCKET sd);

 protected:
  SOCKET getSD() const
  {
    return _sd;
  }

 private:
  friend class InputStream;
  friend class OutputStream;

  SWIutilLogger *_logger;
  SOCKET _sd;
  mutable SWIipAddress *_localAddress;
  mutable SWIipAddress *_remoteAddress;
  bool _created;
  bool _bound;
  bool _connected;
  bool _closed;
  bool _shutIn;
  bool _shutOut;
  int _stmo; // -1==block, 0==poll, >0 == waiting time in secs
  int _rtmo; // -1==block, 0==poll, >0 == waiting time in secs
  OutputStream* _outStream;
  InputStream* _inStream;
};

#endif
