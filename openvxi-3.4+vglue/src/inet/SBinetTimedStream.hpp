
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

#ifndef SBINETTIMEDSTREAM_HPP
#define SBINETTIMEDSTREAM_HPP

/**
 * A SBinetTimedStream is a class that is a wrapper around a
 * SBinetStoppableStream.  The only purpose of the SBinetTimedStream is to set
 * time-out of the wrapped stream and then to invoke the request operation on
 * the wrapped stream.  The wrapped stream are actually responsible for
 * respecting time-outs.
 *
 * @doc <p>
 * Copyright 2004 Vocalocity, Inc.
 *  All Rights Reserved.
 **/

#include "VXIinetStream.hpp"
#include "SWIutilLogger.hpp"
#include "SBinetLog.h"
#include "SBinetStoppable.hpp"
class SBinetStoppableStream;

class SBinetTimedStream: public VXIinetStream, public SWIutilLogger
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SBinetTimedStream(SBinetStoppableStream *aStream,
                    VXIlogInterface *log, VXIunsigned diagLogBase);

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetTimedStream();

  VXIinetResult Open(VXIint flags,
		     const VXIMap* properties,
		     VXIMap* streamInfo);

  VXIinetResult Read(/* [OUT] */ VXIbyte*         pBuffer,
                     /* [IN]  */ VXIulong         nBuflen,
                     /* [OUT] */ VXIulong*        pnRead );

  VXIinetResult Write(/* [IN]  */ const VXIbyte*   pBuffer,
                      /* [IN]  */ VXIulong         nBuflen,
                      /* [OUT] */ VXIulong*        pnWritten);

  VXIinetResult Close();
  /**
    * Disabled copy constructor.
   **/
 private:
  SBinetTimedStream(const SBinetTimedStream&);

  /**
    * Disabled assignment operator.
   **/
 private:
  SBinetTimedStream& operator=(const SBinetTimedStream&);

 private:
  void setDelay(VXIint32 delay);
  bool hasTimedOut() const;

 private:
  SBinetStoppableStream* _stream;

  VXIint32 _timeoutOpen;
  VXIint32 _timeoutIO;
  VXIint32 _timeoutDownload;

  SWITimeStamp *_finalTime;
};
#endif
