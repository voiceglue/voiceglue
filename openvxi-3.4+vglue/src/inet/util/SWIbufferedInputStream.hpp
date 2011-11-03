#ifndef SWIBUFFEREDINPUTSTREAM_HPP
#define SWIBUFFEREDINPUTSTREAM_HPP

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

class SWIUTIL_API_CLASS SWIbufferedInputStream: public SWIfilterInputStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Provides buffering for non-buffered stream.
   *
   * @param stream The stream that is being wrapped.
   * @param bufSize the size of the internal buffer.  This is also the size that will be used to call
   * the stream's read operation.
   * @param lookAhead The amount of look ahead characters that can be looked further.
   * @param ownStream  If <code>true</code>, then the stream is deleted when this reader is deleted.
   *
   **/
 public:
  SWIbufferedInputStream(SWIinputStream * stream,
            int bufSize = 1024,
            int lookAhead = 10,
            bool ownStream = true);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIbufferedInputStream();

  /**
   * Reads one character from the stream.

   * Returns either a positive number that can be cast to a char or a negative
   * number representing an error.  See SWIstream.hpp for error codes.
   **/
 public:
  virtual int read();

  /**
   * Reads the specified number of bytes into the specified buffer.
   *
   * Returns the number of bytes stored in the buffer.
   **/
 public:
  virtual int readBytes(void *buffer, int bufSize);

  /**
   * Reads a line of text.  A line is considered to be terminated by any one
   * of a line feed ('\n'), a carriage return ('\r'), or a carriage return
   * followed immediately by a linefeed.  The carriage return and line feed
   * are extracted from the stream but not stored.
   *
   * returns the number of characters in the string, or a negative number
   * representing the cause of the error. See SWIstream.hpp for error codes.
   * Note that function is lost in the case where the value returned is exactly
   * <code>bufSize - 1</code>.  In this case, it is impossible to know whether the reading stopped
   * because of a line-termination or because the buffer is full.
   *
   **/
 public:
  virtual int readLine(char *buffer, int bufSize);

  /**
   * Reads a line of text and store the result into an OutputStream.
   *
   * Returns the number of characters that have been successfully put into the stream.
   **/
  virtual int readLine(SWIoutputStream& out);

  /**
   * Looks at the stream for look-ahead character.
   *
   * @param offset The offset compared to the current read position in the stream.  A value of 0 means
   * the character that would be returned by the next read().
   *
   * @return    Returns either a positive number that can be cast to a char or a negative
   * number representing an error.  See SWIstream.hpp for error codes.
   **/
  virtual int peek(int offset = 0) const;

  virtual bool isBuffered() const;

  virtual int getLookAhead() const;

 public:
  virtual Result close();

 public:
  virtual Result waitReady(long timeoutMs = -1);

  /**
   * Disabled copy constructor.
   **/
 private:
  SWIbufferedInputStream(const SWIbufferedInputStream&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SWIbufferedInputStream& operator=(const SWIbufferedInputStream&);

 private:
  int fillBuffer(int lowBound = 0);

 private:
  SWIinputStream *_stream;
  int _readSize;
  int _lookAhead;
  bool _ownStream;
  unsigned char *_buffer;
  unsigned char * _end;
  unsigned char *_pos;
  bool _eofSeen;
};
#endif
