
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

#include "TokenList.hpp"

#include <sstream>
#include <algorithm>

TokenList::TokenList(const vxistring &tokens) : std::deque<vxistring>()
{
  addTokens(tokens);
}

void TokenList::addTokens(const vxistring &tokens)
{
  if (!tokens.empty()) {
    // The namelist is a series of whitespace delimited strings.  Read each in
    // and insert it into the deque.
    std::basic_stringstream<VXIchar> namestream(tokens);

#if ((defined(STLPORT) && defined(_STLPORT_VERSION)) || defined(__GNUC__) || defined(_decunix_))
    // G++ 2.95 and 3.0.2 do not fully support istream_iterator.
    while (namestream.good()) {
      vxistring temp;
      namestream >> temp;
      if (!temp.empty()) push_back(temp);
    }
#else
    std::copy(std::istream_iterator<vxistring, VXIchar>(namestream),
              std::istream_iterator<vxistring, VXIchar>(),
              std::back_inserter(*this));
#endif

    // Now sort the deque and return the unique set of names.
    if (!empty()){
      std::sort(begin(), end());
      TokenList::iterator i = std::unique(begin(), end());
      if (i != end()) erase(i, end());
    }
  }
}
