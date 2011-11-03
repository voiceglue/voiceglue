
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

#include <SWIbufferedInputStream.hpp>
#include <SWIbufferedOutputStream.hpp>
#include <SWIsocket.hpp>
#include "SBinetSSLsocket.hpp"
#include "SBinetChannel.h"
#include "SBinetHttpConnection.hpp"

SBinetHttpConnection::SBinetHttpConnection(SBinetURL::Protocol protocol,
                                           const SWIipAddress& ipAddress,
                                           bool usesProxy,
                                           SBinetChannel *channel,
                                           const char *connId):
  _remoteAddress(ipAddress),_socket(NULL),_inputStream(NULL),
  _outputStream(NULL),_channel(channel), _usesProxy(usesProxy),
  _protocol(protocol), _connId(NULL)
{
  if (connId != NULL)
  {
    _connId = new char[strlen(connId) + 1];
    ::strcpy(_connId, connId);
  }
}

SBinetHttpConnection::~SBinetHttpConnection()
{
  close();
  delete [] _connId;
}


SWIstream::Result SBinetHttpConnection::connect(long timeout)
{
  if (_socket != NULL) return SWIstream::SUCCESS;

  switch (_protocol)
  {
   case SBinetURL::HTTP_PROTOCOL:
     _socket = new SWIsocket(SWIsocket::sock_stream, _channel);
     break;
   case SBinetURL::HTTPS_PROTOCOL:
     _socket = new SBinetSSLsocket(SWIsocket::sock_stream, _channel);
     break;
   default:
     _channel->Error(263, NULL);
     return SWIstream::INVALID_ARGUMENT;
  }

  SWIstream::Result rc = _socket->connect(_remoteAddress, timeout);

  if (rc != SWIstream::SUCCESS)
  {
    delete _socket;
    _socket = NULL;
  }

  return rc;
}

SWIinputStream *SBinetHttpConnection::getInputStream()
{
  if (_socket != NULL && _inputStream == NULL)
  {
    SWIinputStream *s = _socket->getInputStream();
    if (s != NULL)
    {
      _inputStream = (s->isBuffered() ?
                      s :
                      new SWIbufferedInputStream(s));
    }
  }
  return _inputStream;
}

SWIoutputStream *SBinetHttpConnection::getOutputStream()
{
  if (_socket != NULL && _outputStream == NULL)
  {
    SWIoutputStream *s = _socket->getOutputStream();
    if (s != NULL)
    {
      _outputStream = (s->isBuffered() ?
                       s :
                       new SWIbufferedOutputStream(s));
    }
  }
  return _outputStream;
}

SWIstream::Result SBinetHttpConnection::close()
{
  if (_socket == NULL) return SWIstream::ILLEGAL_STATE;

  SWIstream::Result rc = _socket->close();

  delete _socket;
  delete _inputStream;
  delete _outputStream;

  _socket = NULL;
  _inputStream = NULL;
  _outputStream = NULL;

  return rc;
}

const SWIipAddress* SBinetHttpConnection::getRemoteAddress()
{
  return &_remoteAddress;
}
