#ifndef SBINETHTTPCONNECTION_HPP
#define SBINETHTTPCONNECTION_HPP

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

#include <SWIstream.hpp>
#include <SWIipAddress.hpp>

class SWIsocket;
class SWIinputStream;
class SWIoutputStream;
class SWIutilLogger;
class SWIipAddress;
class SBinetChannel;

#include "SBinetURL.h"

/**
 * class to encapsulate HTTP connection.
 * @doc <p>
 **/

class SBinetHttpConnection
{

  friend class SBinetChannel;

  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Constructor.
   **/
 private:
  SBinetHttpConnection(SBinetURL::Protocol protocol,
                       const SWIipAddress& ipAddress,
                       bool usesProxy,
                       SBinetChannel *channel,
                       const char *connId = NULL);

 public:
  SWIstream::Result connect(long timeout);
  SWIinputStream *getInputStream();
  SWIoutputStream *getOutputStream();
  SWIstream::Result close();
  const SWIipAddress *getRemoteAddress();

  bool usesProxy() const
  {
    return _usesProxy;
  }

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetHttpConnection();

 public:
  const char *getId() const
  {
    return _connId;
  }

  /**
   * Disabled copy constructor.
   **/
 private:
  SBinetHttpConnection(const SBinetHttpConnection&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SBinetHttpConnection& operator=(const SBinetHttpConnection&);

 private:
  SWIsocket *_socket;
  SWIinputStream *_inputStream;
  SWIoutputStream *_outputStream;
  SBinetChannel *_channel;
  SWIipAddress _remoteAddress;
  bool _usesProxy;
  SBinetURL::Protocol _protocol;
  char *_connId;
};
#endif
