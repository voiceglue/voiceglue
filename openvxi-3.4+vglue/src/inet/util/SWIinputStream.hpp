
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

#ifndef SWIinputStream_HPP
#define SWIinputStream_HPP

#include <SWIstream.hpp>

class SWIoutputStream;

/**
 * Abstract input stream class.  The basic method that sub-classes need to
 * re-implement is the <code>read</code> method.
 *
 *
 * @doc <p>
 * Copyright 2004 Vocalocity, Inc.
 *  All Rights Reserved.
 **/
class SWIUTIL_API_CLASS SWIinputStream: public SWIstream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /// default constructor
 public:
  SWIinputStream()
  {}


  // ------------------------------------------------------------
  /// destructor
 public:
  virtual ~SWIinputStream()
  {}

  //
  // ......... DATA MEMBERS, GETTERS, SETTERS  .......
  //

  //
  // ......... METHODS ..........
  //
  /**
   * Reads some number of bytes from the input stream and stores them into the
   * buffer array <code>data</code>.  @doc The number of bytes actually read
   * is returned as an integer.  In blocking mode, this method blocks until
   * data is available, end of file is detected, or an error is detected.
   * Otherwise it returns immediately after having read available bytes.
   *
   * <p> If <code>data</code> is <code>null</code>, a
   * <code>SWI_STREAM_INVALID_ARGUMENT</code> is returned.  If
   * <code>dataSize</code> of is zero, then no bytes are read and
   * <code>0</code> is returned; otherwise, there is an attempt to read at
   * least one byte. If no byte is available because the stream is at end of
   * file, the value <code>SWI_STREAM_EOF</code> is returned. If the stream
   * is not opened, <code>SWI_STREAM_ILLEGAL_STATE</code> is returned.
   *
   * @param data [out] the buffer into which the data is read.
   * @param dataSize [in] the number of bytes to be read.
   * @return the total number of bytes read into the buffer, or a negative
   * number indicating there is no more data because the end of the stream has
   * been reached, or to indicate an error.
   **/
 public:
  virtual int readBytes(void* data, int dataSize) = 0;

  /**
   * Reads one character from the stream.

   * Returns either a non-negative number that can be cast to a char or a negative
   * number representing an error.  See SWIstream.hpp for error codes.
   * Default implementation calls readBytes with dataSize of 1 and returns the character read.
   * Subclass can re-implement this for better efficiency.
   **/
 public:
  virtual int read();

  /**
   * Skips over and discards <code>n</code> bytes of data from this input
   * stream.  @doc The <code>skip</code> method may end up skipping over some
   * smaller number of bytes, possibly <code>0</code>.  This may result from
   * reaching end of file before <code>n</code> bytes have been skipped.  The
   * actual number of bytes skipped is returned.  If <code>n</code> is
   * negative, no bytes are skipped.  The default implementation just attemps
   * to read up to <code>n</code> bytes into a buffer and discards the buffer.
   * Subclasses can re-implement this method for better efficiency.
   *
   * @param n [in] the number of bytes to be skipped.
   * @return the actual number of bytes skipped, or a negative number
   * indicating an error.
   **/
 public:
  virtual int skip(int n);

  /**
   * Reads a line of text.  A line is considered to be terminated by any one
   * of a line feed ('\n'), a carriage return ('\r'), or a carriage return
   * followed immediately by a linefeed.  The carriage return and line feed
   * are extracted from the stream but not stored.
   *
   * returns the number of characters in the string, or a negative number
   * representing the cause of the error. See SWIstream.hpp for error codes.
   * In any case the string is 0-terminated.  In particular, if we exceed the
   * buffer capacity before reaching the end of line, the BUFFER_OVERFLOW
   * value is returned.
   *
   * If the stream does not support lookAhead, then a NOT_SUPPORTED error is
   * returned.
   *
   **/
 public:
  virtual int readLine(char *buffer, int bufSize);

  /**
   * Reads a line of text and store the result into an OutputStream.
   *
   * Returns the number of characters that have been successfully put into the
   * stream.
   * If the stream does not support lookAhead, then a NOT_SUPPORTED error is
   * returned.
   **/
 public:
  virtual int readLine(SWIoutputStream& out);

  /**
   * Looks at the stream for look-ahead character.
   *
   * @param offset The offset compared to the current read position in the
   * stream.  An offset of 0 means the character that would be returned by the
   * next read().
   *
   * @return Returns either a positive number that can be cast to a char or a
   * negative number representing an error.  If offset is negative or not
   * smaller than the value returned by getLookAhead(), then INVALID_ARGUMENT
   * is returned.

   * Default implementation returns NOT_SUPPORTED.  See SWIstream.hpp for
   * error codes.
   **/
 public:
  virtual int peek(int offset = 0) const;

  /**
   * Returns the maximum look ahead allowed by the stream.  The default
   * implementation returns 0, this method should be re-implemented subclasses
   * that allow look ahead.
   **/
 public:
  virtual int getLookAhead() const;

  /**
   * Returns true if this InputStream is an instance of SWIbufferedStream and
   * can be safely cast to a SWIbufferedStream.  This implementation returns
   * false.
   **/
 public:
  virtual bool isBuffered() const;

  /**
   * Waits until some bytes are available for reading or the specified timeout
   * expires.
   *
   * @param timeoutMs timeout, if less than 0, no timeout, a value of 0 means
   * polling, verify the state of the stream and return immediately.
   *
   * Returns SUCCESS if the stream is ready, TIMED_OUT if the delay has
   * expired or any other Stream::Result value to indicate a failure.
   *
   * Default implementation returns SWIstream::SUCCESS.
   **/
 public:
  virtual Result waitReady(long delayMs = -1);

  /**
   * Closes this input stream and releases any system resources associated
   * with the stream.
   *
   **/
 public:
  virtual Result close() = 0;
};

#endif
