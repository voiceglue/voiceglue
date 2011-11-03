#ifndef SWITIMEWATCH_HPP
#define SWITIMEWATCH_HPP

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

#include "SWIutilHeaderPrefix.h"

#include "SWITimeStamp.hpp"

/**
 * class to facilitate computing elapsed time of operations.
 * @doc <p>
 **/

class SWIUTIL_API_CLASS SWITimeWatch
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SWITimeWatch();

  /**
   * Destructor.
   **/
 public:
  ~SWITimeWatch()
  {}

  // Starts the timer. This sets the reference time from which all new elapsed
  // time are computed.  This does not reset the elapsed time to 0.  This is
  // useful to pause the timer.
 public:
  void start();

  // Stops the timer.
 public:
  void stop();

  // Returns the elapsed time.  If the TimeWatch is in the stopped state,
  // successive calls to getElapsed() will always return the same value.  If
  // the TimeWatch is in the started state, successive calls will return the
  // elapsed time since start() was called.
 public:
  unsigned long getElapsed();

  // Sets the elapsed to 0 and resets the reference time to now.
 public:
  void reset();

 private:
  unsigned long _elapsed;

 private:
  SWITimeStamp _timestamp;
};

#endif
