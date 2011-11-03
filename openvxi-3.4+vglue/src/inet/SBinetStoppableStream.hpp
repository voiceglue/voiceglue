
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

#ifndef SBINETEXPIRABLESTREAM_HPP
#define SBINETEXPIRABLESTREAM_HPP

/**
 * This class represents a SBinetStream that is stoppable.  This means that
 * time-out can be set and inspected for expiration.  However, this class only
 * provides a simple interface for stream to be stopped.  The actual detection
 * of time-out expiration has to be done within the derived classes.
 *
 * @doc <p>
 **/

#include "SBinetStream.hpp"
#include "SBinetStoppable.hpp"
#include "SBinetUtils.hpp"

class SBinetStoppableStream: public SBinetStream, public SBinetStoppable
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SBinetStoppableStream(SBinetURL *url, SBinetStreamType type, VXIlogInterface *log, VXIunsigned diagLogBase):
  SBinetStream(url, type, log, diagLogBase),
  SBinetStoppable()
  {}

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetStoppableStream()
  {}

  /**
    * Disabled copy constructor.
   **/
 private:
  SBinetStoppableStream(const SBinetStoppableStream&);

  /**
    * Disabled assignment operator.
   **/
 private:
  SBinetStoppableStream& operator=(const SBinetStoppableStream&);

  /**
    * MD5 info of the fetched uri for cache stream
   **/  
 public:
  virtual void SetMD5Info(const SBinetMD5 & md5k)
  {
    _md5k = md5k;
  }
  
  virtual const SBinetMD5 & GetMD5Info() const 
  {
    return _md5k;
  }
  
  virtual void ClearMD5Info() 
  {
    _md5k.Clear();  
  }
  
 private:
  SBinetMD5 _md5k;
};
#endif
