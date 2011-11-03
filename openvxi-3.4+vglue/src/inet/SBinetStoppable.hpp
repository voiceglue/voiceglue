
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

#ifndef SBINETEXPIRABLE_HPP
#define SBINETEXPIRABLE_HPP

// This class implements a simple interface to set time-out and to determine
// whether they have expired.

#include "SWITimeStamp.hpp"

class SBinetStoppable
{
 public:
  SBinetStoppable()
  {}

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetStoppable()
  {}

  /**
   * Sets the time-out.  A NULL value removes the current time-out.
   */
 public:
  virtual void setTimeOut(const SWITimeStamp *timeout);

 public:
  bool hasTimedOut() const;

 public:
  void getTimeOut(SWITimeStamp *timeout) const
  {
    *timeout = _expiration;
  }

 public:
  long getDelay() const;

 private:
  SWITimeStamp _expiration;
};

#endif
