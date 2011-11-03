
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

#include <iostream>
#include "VXIvalue.h"

static std::basic_ostream<VXIchar>& operator<<(std::basic_ostream<VXIchar>& os,
                                               const VXIValue * val)
{
  if (val == NULL)
    return os << L"NULL";
  VXIchar tmp[256];
  
  switch (VXIValueGetType(val)) {
  case VALUE_BOOLEAN:
    if(VXIBooleanValue(reinterpret_cast<const VXIBoolean*>(val)) == TRUE)
      return os << L"true";
    else
      return os << L"false";
  case VALUE_INTEGER:
    return os << VXIIntegerValue(reinterpret_cast<const VXIInteger*>(val));
  case VALUE_FLOAT: {
#ifdef WIN32
    _snwprintf(tmp, 256, L"%f", VXIFloatValue(reinterpret_cast<const VXIFloat*>(val)));
#else
    swprintf(tmp, 256, L"%f", VXIFloatValue(reinterpret_cast<const VXIFloat*>(val)));
#endif
    return os << tmp;
  }
#if (VXI_CURRENT_VERSION >= 0x00030000)
  case VALUE_DOUBLE:
#ifdef WIN32
    _snwprintf(tmp, 256, L"%f", VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val)));
#else
    swprintf(tmp, 256, L"%f", VXIDoubleValue(reinterpret_cast<const VXIDouble*>(val)));
#endif
    return os << tmp;
  case VALUE_ULONG:
    return os << VXIULongValue(reinterpret_cast<const VXIULong*>(val));
#endif
  case VALUE_STRING:
    return os << VXIStringCStr(reinterpret_cast<const VXIString *>(val));
  case VALUE_VECTOR:
    {
      const VXIVector * v = reinterpret_cast<const VXIVector *>(val);
      const VXIunsigned len = VXIVectorLength(v);

      os << L"{ ";
      for (VXIunsigned i = 0; i < len; ++i) {
        if (i != 0) os << L", ";
        os << VXIVectorGetElement(v, i);
      }
      return os << L" }";
    }
  case VALUE_MAP:
    {
      const VXIMap * m = reinterpret_cast<const VXIMap *>(val);
      const VXIchar * key;
      const VXIValue * value;

      os << L"{ ";
      if (VXIMapNumProperties(m) != 0) {
        VXIMapIterator * i = VXIMapGetFirstProperty(m, &key, &value);
        os << L"(" << key << L", " << value << L")";
        
        while (VXIMapGetNextProperty(i, &key, &value)
               == VXIvalue_RESULT_SUCCESS)
          os << L", (" << key << L", " << value << L")";

        VXIMapIteratorDestroy(&i);
      }
      return os << L" }";
    }
  default:
    break;
  }

  return os << L"(unprintable result)";
}


static std::basic_ostream<VXIchar>& operator<<(std::basic_ostream<VXIchar>& os,
                                               const VXIMap * val)
{
  return os << reinterpret_cast<const VXIValue *>(val);
}
