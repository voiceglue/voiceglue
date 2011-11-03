
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

#ifndef _COUNTERS_H_
#define _COUNTERS_H_

#include "VXItypes.h"
#include <map>

class EventCounter {
public:
  void Clear()
  { counts.clear(); phaseName.erase(); }

  void ClearIf(const vxistring & phase, bool condition)
  { 
	  if ((phase == phaseName) == condition) 
     { 
	     counts.clear();
        phaseName = phase; 
	  } 
  }

  void Increment(const vxistring & eventName);
  // Increments the count associated with all events.  For instance, given
  // 'error.semantic.bad_name', this will increment 'error', 'error.semantic',
  // and 'error.semantic.bad_name'.

  int GetCount(const vxistring & eventName ) const;

private:
  vxistring phaseName;
  typedef std::map<vxistring, int> COUNTS;
  COUNTS counts;
};


class PromptTracker {
public:
  void Clear()
  { counts.clear(); }

  void Clear(const vxistring & name)
  { COUNTS::iterator i = counts.find(name);
    if (i != counts.end()) counts.erase(i); }

  int Increment(const vxistring & name)
  { COUNTS::iterator i = counts.find(name);
    if (i != counts.end()) return ++(*i).second;
    counts[name] = 1;
    return 1; }

private:
  typedef std::map<vxistring, int> COUNTS;
  COUNTS counts;
};

#endif
