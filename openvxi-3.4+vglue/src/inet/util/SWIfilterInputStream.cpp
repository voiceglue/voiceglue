
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

#include "SWIfilterInputStream.hpp"
#include "SWIinputStream.hpp"
#include "SWIoutputStream.hpp"
#include <string.h>

SWIfilterInputStream::SWIfilterInputStream(SWIinputStream *stream,
                                           bool ownStream):
  _stream(stream),_ownStream(ownStream)
{}

SWIfilterInputStream::~SWIfilterInputStream()
{
  _stream->close();
  if (_ownStream)
  {
    delete _stream;
    _stream = NULL;
  }
}

int SWIfilterInputStream::readBytes(void *buffer, int dataSize)
{
  return _stream->readBytes(buffer, dataSize);
}

int SWIfilterInputStream::read()
{
  return _stream->read();
}

int SWIfilterInputStream::skip(int n)
{
  return _stream->skip(n);
}

int SWIfilterInputStream::readLine(SWIoutputStream& outstream)
{
  return _stream->readLine(outstream);
}

int SWIfilterInputStream::readLine(char *buffer, int bufSize)
{
  return _stream->readLine(buffer, bufSize);
}

int SWIfilterInputStream::peek(int offset) const
{
  return _stream->peek(offset);
}

int SWIfilterInputStream::getLookAhead() const
{
  return _stream->getLookAhead();
}

bool SWIfilterInputStream::isBuffered() const
{
  return _stream->isBuffered();
}

SWIstream::Result SWIfilterInputStream::close()
{
  return _stream->close();
}

SWIstream::Result SWIfilterInputStream::waitReady(long timeoutMs)
{
  return _stream->waitReady(timeoutMs);
}
