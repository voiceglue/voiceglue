
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

#ifndef _ACCESSCONTROL_H_
#define _ACCESSCONTROL_H_

#include "VXItypes.h"
#include <list>

#include <xercesc/dom/DOMDocument.hpp>

/**
 * Handles access-control ProcessingInstruction elements
 * in an XML document fetched via the &lt;data&gt; tag.
 */
class AccessControl
{
public:
  /**
   * @param defAccess The default access used when there
   *                  are no access-control elements, or
   *                  there are no "allow" or "deny"
   *                  attributes.
   */
  AccessControl(bool defAccess);
  AccessControl(bool defAccess, xercesc::DOMDocument *doc);

  /**
   * Add hosts to the allow list.
   * @param allow A space delimited list of host/IP names
   */
  void addAllow(const vxistring &allow);
  /**
   * Add hosts to the denylist.
   * @param deny A space delimited list of host/IP names
   */
  void addDeny(const vxistring &deny);

  /**
   * Determines whether the host in the URL can
   * access the data.
   */
  bool canAccess(const vxistring &url) const;

private:
  class Entry
  {
  public:
    Entry(const vxistring &data);

	bool match(const vxistring &host) const;
	int bestmatch(const vxistring &host) const;

	bool isWild() const { return _isWild; }

  private:
    vxistring _data;
	bool      _isWild;
  };

  typedef std::list<Entry> EntryList;

private:
  vxistring getValue(const vxistring &attr, const vxistring &data) const;
  void addEntries(const vxistring &data, EntryList &list);
  bool parseURL(const vxistring &url, vxistring &proto, vxistring &host) const;

  EntryList _allowList;
  EntryList _denyList;
  bool      _default;
};

#endif
