
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

#include "Counters.hpp"

void EventCounter::Increment(const vxistring & eventName)
{ 
  vxistring::size_type start = 0;
  do {
    vxistring::size_type pos = eventName.find('.', start);
    if (pos == vxistring::npos) pos = eventName.length();
    if (pos != start) { // ignore isolated '.' or '..'
      COUNTS::iterator i = counts.find(eventName.substr(0, pos));
		if (i != counts.end())
			++(*i).second;
		else
			counts[eventName.substr(0, pos)] = 1;
    }
    start = pos + 1;
  } while (start < eventName.length());
}

int EventCounter::GetCount(const vxistring & eventName ) const
{
  if (eventName.empty()) 
	  return 0;

  COUNTS::const_iterator i = counts.find(eventName);
  if (i != counts.end()) 
	  return (*i).second;

  return 0; // This shouldn't happen if Increment was called first.
}
