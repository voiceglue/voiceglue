
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

#ifndef SWIdataOutputStream_HPP
#define SWIdataOutputStream_HPP

#include "SWIoutputStream.hpp"

#define SWI_BUFFER_OUTPUT_STREAM_DEFAULT_SIZE 512

/**
 * This class represents an output stream that writes to a buffer.  The
 * <code>write</code> method causes the buffer to grow if it cannot hold the
 * new data.  Once the stream is completed, the buffer can be retrieved using
 * the <code>getBuffer</code> method and the number of bytes in the buffer is
 * retrieved using the <code>getSize</code> method.  The memory buffer is
 * deleted once the output stream is deleted.  Therefore, the buffer should be
 * copied, if it is required after the stream is deleted.
 *
 * @doc <p> Copyright 2004 Vocalocity, Inc.
 * All Rights Reserved.
 **/
class SWIUTIL_API_CLASS SWIdataOutputStream: public SWIoutputStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  /**
   * Creates an empty SWIdataOutputStream whose initial capacity is equal to
   * the specified capacity.  If the capacity is less than 1, the initial
   * capacity is set to 1.
   *
   * @param capacity The initial capacity required for the internal buffer.
   **/
 public:
  SWIdataOutputStream(int capacity = SWI_BUFFER_OUTPUT_STREAM_DEFAULT_SIZE);

  // ------------------------------------------------------------
  /// destructor
 public:
  virtual ~SWIdataOutputStream();

  //
  // ......... METHODS ..........
  //
  /**
   * Writes <code>bufferSize</code> bytes from the specified buffer.
   *
   * <p> If <code>dataSize</code> is less than <code>0</code>, or
   * <code>data</code> is <code>null</code> and <code>dataSize</code> is not
   * <code>0</code>, then <code>SWI_STREAM_INVALID_ARGUMENT</code> is
   * returned.
   *
   * @param    data   [in] the data to be written.
   * @param    dataSize [in]  the number of bytes to write.
   * @return   the number of bytes written, or a negative number indicating failure.
   **/
 public:
  virtual int writeBytes(const void* data, int dataSize);

  /**
   * Returns a pointer to the internal buffer of the stream.  Since only a
   * pointer is returned, the pointer return should not be modified.
   *
   * @return a pointer to the internal buffer of the stream.
   **/
 public:
  const void *getBuffer() const
  {
    return _buffer;
  }

  /**
   * Returns the buffer as a C-string with a NULL character at the end.
   **/
  const char *getString() const;

 public:
  /**
   * Returns the number of bytes in the internal buffer of the stream.
   *
   * @return the number of bytes in the internal buffer of the stream.
   **/
  int getSize() const
  {
    return _bufferSize;
  }

 public:
  /**
   * Resets the size of the buffer to 0.
   **/
  void reset()
  {
    _bufferSize = 0;
  }

 public:
  virtual Result close();
  virtual bool isBuffered() const;

 private:
  Result ensureCapacity(int requiredCapacity) const;

  // members.
  /**
   * The internal buffer for the stream.
   **/
 private: mutable char *_buffer;

  /**
   *  The current capacity of the buffer.
   **/
 private: mutable int _capacity;

  /**
   * The number of bytes written so far into the buffer.
   **/
 private: mutable int _bufferSize;

  //
  // ......... operators  ........
  //

};
#endif
