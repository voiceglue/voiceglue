
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

#include <SWIdataOutputStream.hpp>
#include <cstddef>
#include <memory.h>

SWIdataOutputStream::SWIdataOutputStream(int initialCapacity):
  _bufferSize(0)
{
  _capacity = initialCapacity;
  if (_capacity < 1) _capacity = 1;
  _buffer = new char[_capacity];
}

SWIdataOutputStream::~SWIdataOutputStream()
{
  delete [] _buffer;
}

/**
 * @see SWIdataOutputStream::write documentation.  Warning, this method
 * is not thread-safe.  Only one thread will write to the stream.  If it is
 * not the case, one should derive a sub-class from this one, and re-implement
 * its write method.
 **/
int SWIdataOutputStream::writeBytes(const void* data,
                                    int dataSize)
{
  // Nothing to write.
  if (dataSize == 0)
    return 0;

  // Inconsistent data and dataSize
  if (dataSize < 0 || data == NULL)
    return INVALID_ARGUMENT;

  // No existing buffer.  Can't write.
  if (_buffer == NULL)
  {
    return WRITE_ERROR;
  }

  int requiredCapacity = dataSize + _bufferSize;

  Result rc = ensureCapacity(requiredCapacity);
  if (rc != SUCCESS) return rc;

  // copy the bytes from data into the dataBuffer.
  ::memcpy(_buffer + _bufferSize, data, dataSize);

  // set the new size of the buffer.
  _bufferSize = requiredCapacity;

  return dataSize;
}

SWIstream::Result SWIdataOutputStream::ensureCapacity(int requiredCapacity) const
{
  if (requiredCapacity > _capacity)
  {
    // Make the capacity twice as much as the required capacity.  This
    // ensures an amortized linear running time.
    int oldCapacity = _capacity;
    _capacity = requiredCapacity * 2;

    // Create a new buffer to hold the data.
    char *tmpBuffer = new char[_capacity];
    if (tmpBuffer == NULL)
    {
      _capacity = oldCapacity;
      return WRITE_ERROR;
    }

    // transfer the data from the existing buffer to the new buffer, and make
    // the new buffer the current one.
    ::memcpy(tmpBuffer, _buffer, _bufferSize);
    delete [] _buffer;
    _buffer = tmpBuffer;
  }
  return SUCCESS;
}

const char * SWIdataOutputStream::getString() const
{
  if (ensureCapacity(_bufferSize + 1) != SUCCESS)
    return NULL;

  _buffer[_bufferSize] = '\0';

  return _buffer;
}

SWIstream::Result SWIdataOutputStream::close()
{
  return SUCCESS;
}

bool SWIdataOutputStream::isBuffered() const
{
  return true;
}
