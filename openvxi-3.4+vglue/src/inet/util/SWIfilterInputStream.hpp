#ifndef SWIFILTERINPUTSTREAM_HPP
#define SWIFILTERINPUTSTREAM_HPP

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

/**
 * Class used to filter input stream.  The basic idea is that a
 * SWIfilterInputstream is a wrapper around a SWIinputStream.  This allows to
 * add functionality to existing SWInputStream classes.  The default
 * implementation of all methods of the SWIfilterInputStream is to invoke the
 * corresponding method of the wrapped SWIinputStream

 **/

class SWIUTIL_API_CLASS SWIfilterInputStream: public SWIinputStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SWIfilterInputStream(SWIinputStream *stream, bool ownStream = true);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIfilterInputStream();

 public:
  virtual int readBytes(void* data, int dataSize);

 public:
  virtual int read();

 public:
  virtual int skip(int n);

 public:
  virtual int readLine(char *buffer, int bufSize);

 public:
  virtual int readLine(SWIoutputStream& out);

 public:
  virtual int peek(int offset = 0) const;

 public:
  virtual int getLookAhead() const;

 public:
  virtual bool isBuffered() const;

 public:
  virtual Result waitReady(long timeoutMs = -1);

 public:
  virtual Result close();

  /**
   * Disabled copy constructor.
   **/
 private:
  SWIfilterInputStream(const SWIfilterInputStream&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SWIfilterInputStream& operator=(const SWIfilterInputStream&);

 private:
  SWIinputStream *_stream;
  bool _ownStream;
};
#endif
