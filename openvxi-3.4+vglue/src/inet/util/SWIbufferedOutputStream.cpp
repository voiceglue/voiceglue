
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

#include "SWIbufferedOutputStream.hpp"
#include <cstdio>

SWIbufferedOutputStream::SWIbufferedOutputStream(SWIoutputStream *outStream,
                                                 int bufferSize,
                                                 bool ownStream):
  SWIfilterOutputStream(outStream, ownStream),
  _buffer(NULL), _pos(NULL), _end(NULL)
{
  if (bufferSize < 1) bufferSize = 1;
  _buffer = new char[bufferSize];
  _pos = _buffer;
  _end = _pos + bufferSize;
}

SWIbufferedOutputStream::~SWIbufferedOutputStream()
{
  flush();
  if (isOwner()) close();
}

SWIstream::Result SWIbufferedOutputStream::close()
{
  Result rc1 = flush();
  Result rc2 = SWIfilterOutputStream::close();

  delete [] _buffer;
  _pos = _end = _buffer = NULL;
  return rc1 == SUCCESS ? rc2 : rc1;
}


int SWIbufferedOutputStream::writeBytes(const void *data, int dataSize)
{
  if (_buffer == NULL) return SWIstream::ILLEGAL_STATE;
  const char *p = static_cast<const char *>(data);
  const char *q = p + dataSize;
  int nbWritten = 0;

  while (p < q)
  {
    while (_pos < _end && p < q)
    {
      *_pos++ = *p++;
      nbWritten++;
    }

    SWIstream::Result rc;
    if (p < q && (rc = flushBuffer()) != SWIstream::SUCCESS)
      return rc;
  }
  return nbWritten;
}

SWIstream::Result SWIbufferedOutputStream::printString(const char *s)
{
  if (_buffer == NULL) return SWIstream::ILLEGAL_STATE;

  const char *p = s;

  while (*p)
  {
    while (_pos < _end && *p) *_pos++ = *p++;

    SWIstream::Result rc;
    if (*p && (rc = flushBuffer()) != SWIstream::SUCCESS)
      return rc;
  }
  return SWIstream::SUCCESS;
}

SWIstream::Result  SWIbufferedOutputStream::printChar(char c)
{
  if (_buffer == NULL) return SWIstream::ILLEGAL_STATE;

  SWIstream::Result rc;
  if (_pos == _end && (rc = flushBuffer()) != SWIstream::SUCCESS)
    return rc;

  *_pos++ = c;
  return SWIstream::SUCCESS;
}

SWIstream::Result SWIbufferedOutputStream::flush()
{
  if (_buffer == NULL) return SWIstream::ILLEGAL_STATE;
  return flushBuffer();
}

bool SWIbufferedOutputStream::isBuffered() const
{
  return true;
}

SWIstream::Result SWIbufferedOutputStream::flushBuffer()
{
  int len = _pos - _buffer;
  _pos = _buffer;

  if (len == 0)
    return SWIstream::SUCCESS;

  int rc = SWIfilterOutputStream::writeBytes(_buffer, len);

  if (len == rc)
    return SWIstream::SUCCESS;

  if (rc >= 0)
    return SWIstream::WRITE_ERROR;

  return SWIstream::Result(rc);
}
