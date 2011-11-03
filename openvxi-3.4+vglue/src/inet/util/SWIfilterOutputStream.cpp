
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

#include "SWIfilterOutputStream.hpp"
#include "SWIutilInternal.h"

// SWIfilterOutputStream::SWIfilterOutputStream
// Refer to SWIfilterOutputStream.hpp for doc.
SWIfilterOutputStream::SWIfilterOutputStream(SWIoutputStream *outStream,
                                             bool ownStream):
  _stream(outStream), _ownStream(ownStream)
{}

// SWIfilterOutputStream::~SWIfilterOutputStream
// Refer to SWIfilterOutputStream.hpp for doc.
SWIfilterOutputStream::~SWIfilterOutputStream()
{
  if (_ownStream)
  {
    close();
    delete _stream;
    _stream = NULL;
  }
}

int SWIfilterOutputStream::writeBytes(const void *data, int dataSize)
{
  return _stream->writeBytes(data, dataSize);
}


bool SWIfilterOutputStream::isBuffered() const
{
  return _stream->isBuffered();
}


SWIstream::Result SWIfilterOutputStream::printString(const char *s)
{
  return _stream->printString(s);
}


SWIstream::Result SWIfilterOutputStream::printChar(char c)
{
  return _stream->printChar(c);
}


SWIstream::Result SWIfilterOutputStream::flush()
{
  return _stream->flush();
}

SWIstream::Result SWIfilterOutputStream::close()
{
  return _stream->close();
}

SWIstream::Result SWIfilterOutputStream::waitReady(long timeoutMs)
{
  return _stream->waitReady(timeoutMs);
}
