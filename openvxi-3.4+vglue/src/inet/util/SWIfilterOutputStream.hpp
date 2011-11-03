#ifndef SWIFILTEROUTPUTSTREAM_HPP
#define SWIFILTEROUTPUTSTREAM_HPP

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

/**
 * Class used to filter output stream.  The basic idea is that a
 * SWIfilterOutputstream is a wrapper around a SWIoutputStream.  This allows to
 * add functionality to existing SWInputStream classes.  The default
 * implementation of all methods of the SWIfilterOutputStream is to invoke the
 * corresponding method of the wrapped SWIoutputStream
 *
 *
 **/

#include "SWIoutputStream.hpp"

class SWIUTIL_API_CLASS SWIfilterOutputStream: public SWIoutputStream
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SWIfilterOutputStream(SWIoutputStream *outStream,
                        bool ownStream = true);

  /**
   * Destructor.
   **/
 public:
  virtual ~SWIfilterOutputStream();

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

 public:
  virtual SWIstream::Result waitReady(long timeoutMs = -1);

 protected:
  bool isOwner() const
  {
    return _ownStream;
  }

  /**
   * Disabled copy constructor.
   **/
 private:
  SWIfilterOutputStream(const SWIfilterOutputStream&);

  /**
   * Disabled assignment operator.
   **/
 private:
  SWIfilterOutputStream& operator=(const SWIfilterOutputStream&);

 private:
  const bool _ownStream;
  SWIoutputStream *_stream;
};
#endif
