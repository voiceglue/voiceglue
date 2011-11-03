#ifndef SWITIMESTAMP_HPP
#define SWITIMESTAMP_HPP

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

#include <time.h>

class SWIUTIL_API_CLASS SWITimeStamp
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.  The current timestamp is initialized to 0.0.
   **/
 public:
  SWITimeStamp();

  /**
   * Destructor.
   **/
 public:
  ~SWITimeStamp();

  /**
   * Assigns the current system time to this timestamp.
   **/
 public:
   void setTimeStamp();

  /**
   * Reset the timestamp to current system time then add the
   * delay that previously set by addDelay()
   **/
 public:  
  void resetTimeStamp(void);

  /**
   * Adds this number of milliseconds to the current timestamp.
   **/
 public:
  void addDelay(unsigned long msecs);  

  /**
   * return milliseconds value of the previously call to addDelay()
   **/
 public:  
  unsigned long getAddedDelay(void);

  /**
   * Returns the number of milliseconds between this timestamp and the
   * timestamp passed as an argument.
   */
 public:
  long delta(const SWITimeStamp& timestamp) const;

 public:
  time_t getSecs() const;

  /**
   * Returns -1,0,1 depending on whether this timestamp is smaller than, equal
   * to, or greater than the specified timestamp.
   **/
 public:
  int compare(const SWITimeStamp& timestamp) const;

 public:
  unsigned short getMsecs() const;

 public:
  void reset();

 private:
  time_t _secs;
  unsigned short _msecs;
  unsigned long  _addedDelay;
};

#endif
