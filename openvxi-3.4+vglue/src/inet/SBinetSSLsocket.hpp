#ifndef SBINETSSLSOCKET_HPP
#define SBINETSSLSOCKET_HPP

/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#include "SWIsocket.hpp"
#include <openssl/ssl.h>

/**
 * This class encapsulates client-side SSL operations.
 **/
class SBinetSSLsocket: public SWIsocket
{
 public:
  static int initialize();
  static int shutdown();

  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SBinetSSLsocket(Type socktype = sock_stream, SWIutilLogger *logger = NULL);

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetSSLsocket();

  virtual SWIstream::Result connect(const SWIipAddress& endpoint, long timeout = -1);

  virtual int read(void* buf, int len);
  virtual int recv(void* buf, int len, int msgf=0);
  virtual int recvfrom(SWIipAddress& sa, void* buf, int len, int msgf=0);

  virtual int write	(const void* buf, int len);
  virtual int send	(const void* buf, int len, int msgf=0);
  virtual int sendto	(const SWIipAddress& sa, const void* buf, int len, int msgf=0);

  virtual SWIstream::Result shutdown(shuthow sh);
  virtual SWIstream::Result close();

  /**
   * Disabled copy constructor.
   **/
 private:
  SBinetSSLsocket(const SBinetSSLsocket&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SBinetSSLsocket& operator=(const SBinetSSLsocket&);

 private:
  static SSL_CTX *_ctx;
  static const SSL_METHOD *_meth;
  SSL *_ssl;

};
#endif
