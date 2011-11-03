#ifndef SWIBUFFEREDOUTPUTSTREAM_HPP
#define SWIBUFFEREDOUTPUTSTREAM_HPP

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

class SWIUTIL_API_CLASS SWIbufferedOutputStream: public SWIfilterOutputStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Creates a Writer wrapped around the stream.
   *
   * @param stream The OutputStream to be wrapped by the writer.
   * @param the size of the internal buffer to

   **/
 public:
  SWIbufferedOutputStream(SWIoutputStream *outStream,
                          int bufferSize = 1024,
                          bool ownStream = true);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIbufferedOutputStream();

 public:
  virtual int writeBytes(const void *data, int dataSize);

 public:
  bool isBuffered() const;

 public:
  virtual SWIstream::Result printString(const char *s);

 public:
  virtual SWIstream::Result printChar(char c);

 public:
  virtual SWIstream::Result flush();

 public:
  virtual SWIstream::Result close();

 private:
  SWIstream::Result flushBuffer();

  /**
   * Disabled copy constructor.
   **/
 private:
  SWIbufferedOutputStream(const SWIbufferedOutputStream&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SWIbufferedOutputStream& operator=(const SWIbufferedOutputStream&);

 private:
  char *_buffer;
  char *_end;
  char *_pos;
};
#endif
