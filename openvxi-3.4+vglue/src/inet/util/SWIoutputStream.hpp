#ifndef SWIOUTPUTSTREAM_HPP
#define SWIOUTPUTSTREAM_HPP

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

#include "SWIstream.hpp"

/**
 ** Abstract class representing output stream classes.
 ** @doc <p>
 **/
class SWIUTIL_API_CLASS SWIoutputStream: public SWIstream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /// default constructor
public:
  SWIoutputStream();

  // ------------------------------------------------------------
  /// destructor
public:
  virtual ~SWIoutputStream();

  //
  // ......... METHODS ..........
  //
  /**
   * Writes <code>bufferSize</code> bytes from the specified buffer.
   *
   * <p>
   * If <code>buffer</code> is <code>null</code>,
   * <code>SWI_STREAM_INVALID_ARGUMENT</code> is returned.
   * <p>
   *
   * @param  buffer      [in] the data to be written.
   * @param  bufferSize  [in] the number of bytes to write.
   * @return the number of bytes written, or a negative number indicating failure.
   **/
public:
  virtual int writeBytes(const void* buffer, int bufferSize) = 0;

  /**
   * Flushes this output stream and forces any buffered output bytes to be
   * written out. The general contract of <code>flush</code> is that calling
   * it is an indication that, if any bytes previously written have been
   * buffered by the implementation of the output stream, such bytes should
   * immediately be written to their intended destination.

   * The default implementation does nothing returns
   * <code>SWI_STREAM_SUCCESS</code> Sub-classes implementing buffering should
   * re-implement this method.  <p>
   *
   *
   * @return <code>SWI_STREAM_SUCCESS</code> if succes, negative value if
   * failure.
   **/
public:
  virtual SWIstream::Result flush();

  /** Indicates whether this stream is buffered or not.  Default implementation
   * returns false.  Sub classes using buffering should reimplemnent this mehod
   * to return true.
   **/
 public:
  virtual bool isBuffered() const;

 public:
  virtual SWIstream::Result printString(const char *s);

 public:
  virtual SWIstream::Result printChar(char c);

 public:
  virtual SWIstream::Result printInt(int i);

  /**
   * Waits until the stream is ready for writing bytes or the specified
   * timeout expires.
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
  virtual SWIstream::Result waitReady(long timeoutMs = -1);

  /**
   * Closes this output stream and releases any system resources
   * associated with this stream. The general contract of <code>close</code>
   * is that it closes the output stream. A closed stream cannot perform
   * output operations.
   * <p>
   * The default implementation just returns <code>SWI_STREAM_SUCCESS</code>
   *
   * @return SWI_STREAM_SUCCESS if success, negative value if failure.
   **/
public:
  virtual Result close() = 0;
};
#endif
