
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

#include "SWIbufferedInputStream.hpp"
#include "SWIoutputStream.hpp"
#include <string.h>

SWIbufferedInputStream::SWIbufferedInputStream(SWIinputStream *stream,
                                     int bufSize, int lookAhead,
                                     bool ownStream):
  SWIfilterInputStream(stream, ownStream),
  _eofSeen(false), _buffer(NULL)
{
  _lookAhead = (lookAhead < 1) ? 1 : lookAhead;
  if (bufSize < 1) bufSize = 1;
  _readSize = bufSize > _lookAhead ? bufSize : _lookAhead;
  _pos = _end = _buffer =
    new unsigned char[bufSize + _lookAhead - 1];
}

SWIbufferedInputStream::~SWIbufferedInputStream()
{
  close();
}

int SWIbufferedInputStream::read()
{
  if (_buffer == NULL) return ILLEGAL_STATE;
  if (_pos < _end) return *_pos++;

  int rc = fillBuffer();
  if (rc < 0) return rc;
  rc = *_pos++;
  return rc;
}

int SWIbufferedInputStream::readBytes(void *data, int dataSize)
{
  if (_buffer == NULL) return ILLEGAL_STATE;
  if (dataSize  < 0 ) return INVALID_ARGUMENT;
  if (data == NULL && dataSize > 0) return INVALID_ARGUMENT;

  unsigned char *p0 = static_cast<unsigned char *>(data);
  unsigned char *p = p0;
  unsigned char *q = p0 + dataSize;
  int rc = SWIstream::SUCCESS;

  for (;;)
  {
    while (_pos < _end && p < q) *p++ = *_pos++;

    // Have we read the required number of bytes?
    if (p == q) break;

    // Ran out of bytes in our buffer.  Only fill the buffer if that would
    // not block and we haven't read a byte yet.
    if (p > p0 && SWIfilterInputStream::waitReady(0) != SUCCESS) break;

    // Bail out if an error occurs while filling the buffer.
    if ((rc = fillBuffer()) < 0) break;
  }

  int nbRead = p - p0;
  return nbRead > 0 ? nbRead : rc;
}

int SWIbufferedInputStream::peek(int offset) const
{
  if (offset < 0 || offset >= _lookAhead)
    return INVALID_ARGUMENT;

  if (_buffer == NULL) return ILLEGAL_STATE;
  if (_end - _pos > offset) return _pos[offset];

  int remaining =
    const_cast<SWIbufferedInputStream *>(this)->fillBuffer(offset);

  if (remaining < 0) return remaining;
  if (remaining > offset) return _pos[offset];
  return END_OF_FILE;
}

int SWIbufferedInputStream::getLookAhead() const
{
  return _lookAhead;
}

int SWIbufferedInputStream::readLine(SWIoutputStream& outstream)
{
  if (_buffer == NULL) return ILLEGAL_STATE;
  int rc = 0;
  int nbWritten = 0;

  for (;;)
  {
    unsigned char *p = _pos;

    while (p < _end && *p != '\r' && *p != '\n') p++;

    int len = p - _pos;

    if (len > 0)
    {
      if ((rc = outstream.writeBytes(_pos, len)) != len)
        break;

      nbWritten += len;
      _pos = p;
    }

    if (_pos < _end)
    {
      if (*_pos == '\r')
      {
        _pos++;
        if ((rc = fillBuffer()) < 0)
          break;
      }
      if (*_pos == '\n')
        _pos++;
      break;
    }
    else if ((rc = fillBuffer()) < 0)
      break;
  }

  if (nbWritten == 0)
    return rc > 0 ? 0 : rc;

  return nbWritten;
}

int SWIbufferedInputStream::readLine(char *buffer, int bufSize)
{
  if (_buffer == NULL) return ILLEGAL_STATE;
  int rc = 0;
  char *p = buffer;
  char *q = buffer + bufSize - 1; // keep one byte for '\0'

  while (p < q)
  {
    while (_pos < _end && p < q)
    {
      switch (*_pos)
      {
       case '\r':
         _pos++;
         rc = fillBuffer();
         if (rc <= 0) goto terminateString;
         if (*_pos != '\n') goto terminateString;
         // no break: intentional
       case '\n':
         _pos++;
         goto terminateString;
       default:
         *p++ = *_pos++;
         break;
      }
    }
    rc = fillBuffer();
    if (rc <= 0) break;
  }

 terminateString:
  *p = '\0';

  if (p == q)
    return BUFFER_OVERFLOW;

  int len = p - buffer;
  if (len == 0)
    return rc > 0 ? 0 : rc;

  return len;
}

bool SWIbufferedInputStream::isBuffered() const
{
  return true;
}

SWIstream::Result SWIbufferedInputStream::waitReady(long timeoutMs)
{
  if (_buffer == NULL) return ILLEGAL_STATE;
  if (_pos < _end) return SUCCESS;
  return SWIfilterInputStream::waitReady(timeoutMs);
}

SWIstream::Result SWIbufferedInputStream::close()
{
  delete [] _buffer;
  _end = _pos = _buffer = NULL;
  return SWIfilterInputStream::close();
}

/**
 * Returns the number of bytes available in the buffer.
 **/
int SWIbufferedInputStream::fillBuffer(int lowBound)
{
  int remaining = _end - _pos;

  // If there are enough remaining characters, return them so that we won't
  // block trying to fill the look-ahead buffer.
  if (remaining > lowBound) return remaining;

  if (remaining == 0 && _eofSeen) return END_OF_FILE;

  // shift data at the beginning of the buffer.
  memmove(_buffer, _pos, remaining);

  _pos = _buffer;
  _end = _pos + remaining;

  // Now read from the stream.
  int rc = SWIfilterInputStream::readBytes(_end, _readSize - remaining);

  switch (rc)
  {
   case END_OF_FILE:
     _eofSeen = true;
     return remaining;
   case 0:
     // This should not really happen, it should either return TIMED_OUT or
     // WOULD_BLOCK.
     return TIMED_OUT;
   default:
     if (rc > 0)
     {
       _end += rc;
       return remaining + rc;
     }
     return rc;
  }
}
