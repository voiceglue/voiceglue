
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

#if _MSC_VER >= 1100    // Visual C++ 5.x
#pragma warning( disable : 4786 4503 )
#endif

#include "SWIoutputStream.hpp"
#include <cstdio>
#include <cstring>

// SWIoutputStream::SWIoutputStream
// Refer to SWIoutputStream.hpp for doc.
SWIoutputStream::SWIoutputStream()
{}

// SWIoutputStream::~SWIoutputStream
// Refer to SWIoutputStream.hpp for doc.
SWIoutputStream::~SWIoutputStream()
{}

SWIstream::Result SWIoutputStream::printString(const char *s)
{
  int len = strlen(s);
  int rc = writeBytes(s, len);
  if (rc < 0) return Result(rc);
  if (rc == len) return SUCCESS;
  return WRITE_ERROR;
}

SWIstream::Result SWIoutputStream::printChar(char c)
{
  int rc = writeBytes(&c, sizeof c);
  if (rc < 0) return Result(rc);
  return rc == sizeof c ? SUCCESS : WRITE_ERROR;
}


SWIstream::Result SWIoutputStream::printInt(int i)
{
  char tmp[20];  // should be more than enough for any 32 bit integer.
  ::sprintf(tmp, "%d", i);
  return printString(tmp);
}

SWIstream::Result SWIoutputStream::flush()
{
  return SUCCESS;
}

bool SWIoutputStream::isBuffered() const
{
  return false;
}

SWIstream::Result SWIoutputStream::waitReady(long timeoutMs)
{
  return SUCCESS;
}
