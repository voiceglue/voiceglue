
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

#include "AccessControl.hpp"
#include "TokenList.hpp"

#include <algorithm>

#include "XMLChConverter.hpp"
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMProcessingInstruction.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

AccessControl::AccessControl(bool defAccess) : _default(defAccess)
{
}

AccessControl::AccessControl(bool defAccess, xercesc::DOMDocument *doc) : _default(defAccess)
{
  DOMNodeList *nl = doc->getChildNodes();
  if (nl && nl->getLength()){
    int len = nl->getLength();
    for (int i = 0; i < len; i++) {
      DOMNode *node = nl->item(i);
	  if (node->getNodeType() == DOMNode::PROCESSING_INSTRUCTION_NODE) {
        DOMProcessingInstruction *pi = reinterpret_cast<DOMProcessingInstruction*>(node);
        if (pi) {
          vxistring target(XMLChToVXIchar(pi->getTarget()).c_str());
          if (target == L"access-control") {
			vxistring data(XMLChToVXIchar(pi->getData()).c_str());
			addAllow(getValue(L"allow", data));
			addDeny(getValue(L"deny", data));
          }
        }
      }
    }
  }
}

vxistring AccessControl::getValue(const vxistring &key, const vxistring &data) const
{
  // find attr
  int keyStart = data.find( key );
  if (keyStart == vxistring::npos) return L"";

  // set index to start of value (not including opening quote)
  int valueStart = keyStart + key.length() + 2;

  // find end of value by finding matching end quote
  vxistring::value_type quote = data[valueStart - 1];
  int valueEnd = data.find(quote, valueStart);
  if (valueEnd == vxistring::npos)
      valueEnd = data.length();

  return data.substr(valueStart, valueEnd - valueStart);
}

void AccessControl::addAllow(const vxistring &allow)
{
  addEntries(allow, _allowList);
}

void AccessControl::addDeny(const vxistring &deny)
{
  addEntries(deny, _denyList);
}

void AccessControl::addEntries(const vxistring &data, EntryList &list)
{
  if (data.empty()) 
    return;
  TokenList el(data);
  TokenList::iterator i;
  for (i = el.begin(); i != el.end(); ++i)
    list.push_back(Entry(*i));
}

bool AccessControl::canAccess(const vxistring &url) const
{
  // treat missing/empty "allow" and "deny" the same as 
  // as missing <?access-control?> element.
  if (_allowList.empty() && _denyList.empty())
    return _default;

  // this should never happen, but without a URL, there
  // is no way to validate.
  if (url.empty())
    return false;

  // get hostname and protocol
  vxistring proto;
  vxistring host;
  if (!parseURL(url, proto, host))
    return false;

  // always allow file access
  if (proto == L"file")
    return true;

  // deny, exact
  EntryList::const_iterator i = _denyList.begin();
  for (; i != _denyList.end(); i++){
    if ((*i).match(host))
      return false;
  }

  // allow, exact
  i = _allowList.begin();
  for (; i != _allowList.end(); i++){
    if ((*i).match(host))
      return true;
  }

  // deny, bestmatch
  int bestDeny = 0;
  i = _denyList.begin();
  for (; i != _denyList.end(); i++){
    int score = (*i).bestmatch(host);
	if (score > bestDeny) bestDeny = score;
  }

  // allow, bestmatch
  int bestAllow = 0;
  i = _allowList.begin();
  for (; i != _allowList.end(); i++){
    int score = (*i).bestmatch(host);
	if (score > bestAllow) bestAllow = score;
  }

  if (bestAllow > bestDeny)
    return true;
  return false;
}

bool AccessControl::parseURL(const vxistring &url, vxistring &proto, vxistring &host) const
{
  proto.clear();
  host.clear();

  // get protocol
  int pos = url.find(':');
  if (pos == vxistring::npos)
    return false;
  proto = url.substr(0,pos);

  if (proto == L"file")
    return true;
  if (proto.length() == 1){
    proto = L"file";
	return true;
  }

  // find beginning of hostname
  ++pos;
  VXIchar c;
  do {
    c = url[pos];
	if (c == L'/') pos++;
  } while(c!=0 && c==L'/');

  // get host
  while(url[pos] != ':' && url[pos] != '/' && url[pos] != 0)
    host.append(1,url[pos++]);

  // convert to lowercase
  std::transform(host.begin(), host.end(), host.begin(), tolower);

  return true;
}

AccessControl::Entry::Entry(const vxistring &data)
{
  _isWild = (data[0] == L'*');
  if (!_isWild)
    _data = data;
  else {
	_data = data.substr(1);
	if ( _data.length() > 0 && _data[0] != '.')
      _data = L"." + _data;
  }

  std::transform(_data.begin(), _data.end(), _data.begin(), tolower );
}

bool AccessControl::Entry::match(const vxistring &host) const
{
  if (_isWild) return false;
  return _data == host;
}

// returns a "score" between 0 and 100, which 0 meaning no
// match at all, and 100 being a exact.
//
// given an "allow" of "*.example.com" and a "deny" of "*.visitors.example.com", then
// a URL of "mach1.visitors.example.com" would give a score of 46 against the "allow",
// and a score of 80 against the deny list.
int AccessControl::Entry::bestmatch(const vxistring &host) const
{
  if (!_isWild)
    return 0;

  // no data would mean a spec of "*", in which case
  // we return a score of 1.
  if (_data.length() == 0)
    return 1;

  int score = 0;
  int pos = host.find(_data);
  if (pos != vxistring::npos){
    int len = host.length();
    score = ((len - pos) * 100) / len;
  }

  return score;
}
