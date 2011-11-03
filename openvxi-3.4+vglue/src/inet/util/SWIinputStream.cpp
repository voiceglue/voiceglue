
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

#include "SWIinputStream.hpp"
#include "SWIoutputStream.hpp"

int SWIinputStream::read()
{
  unsigned char c;
  int rc = readBytes(&c, sizeof c);
  if (rc != sizeof c) return rc;
  return c;
}

int SWIinputStream::skip(int n)
{
  if (n < 0)
    return INVALID_ARGUMENT;

  if (n == 0) return 0;

  char *tmp = new char[n];
  int nbSkip = readBytes(tmp, n);
  delete [] tmp;
  return nbSkip;
}

int SWIinputStream::readLine(SWIoutputStream& outstream)
{
  int rc = getLookAhead();

  if (rc == 0)
    return NOT_SUPPORTED;

  if (rc < 0)
    return rc;

  int nbWritten = 0;
  int c = 0;

  for (;;)
  {
    if ((c = read()) < 0)
    {
      rc = c;
      break;
    }

    switch (c)
    {
     case '\r':
       if ((c = peek()) < 0 || (c == '\n' && (c = read()) < 0))
         rc = c;
       // no break: intentional
     case '\n':
       goto end;
       break;
     default:
       outstream.printChar(c);
       nbWritten++;
       break;
    }
  }
 end:

  if (nbWritten == 0)
    return rc > 0 ? 0 : rc;

  return nbWritten;
}

int SWIinputStream::readLine(char *buffer, int bufSize)
{
  int rc = getLookAhead();

  if (rc == 0)
    return NOT_SUPPORTED;

  if (rc < 0)
    return rc;

  char *p = buffer;
  char *q = buffer + bufSize - 1; // keep one byte for '\0'
  int c = 0;

  while (p < q)
  {
    if ((c = read()) < 0)
    {
      rc = c;
      break;
    }

    switch (c)
    {
     case '\r':
      if ((c = peek()) < 0 || (c == '\n' && (c = read()) < 0))
        rc = c;
      // no break: intentional
     case '\n':
       goto end;
       break;
     default:
       *p++ = c;
       break;
    }
  }
 end:
  *p = '\0';

  if (p == q)
    return BUFFER_OVERFLOW;

  int len = p - buffer;
  if (len == 0)
    return rc > 0 ? 0 : rc;

  return len;
}

int SWIinputStream::peek(int offset) const
{
  return NOT_SUPPORTED;
}

int SWIinputStream::getLookAhead() const
{
  return 0;
}

bool SWIinputStream::isBuffered() const
{
  return false;
}

SWIstream::Result SWIinputStream::waitReady(long timeoutMs)
{
  return SUCCESS;
}
