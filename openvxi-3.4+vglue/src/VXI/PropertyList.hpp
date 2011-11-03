
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

#include "VXIvalue.h"                  // for VXIMap
#include <map>
#include <vector>
#include "CommonExceptions.hpp"

class VXMLElement;
class SimpleLogger;

enum PropertyLevel {
  DEFAULTS_PROP = 0,
  SESSION_PROP  = 1,
  APP_PROP      = 2,
  DOC_PROP      = 3,
  DIALOG_PROP   = 4,
  FIELD_PROP    = 5,
  LAST_PROP     = 6        // this one should always be last in the list
};


class PropertyList {
public:
  static const VXIchar * const BaseURI;
  static const VXIchar * const Language;
  static const VXIchar * const AbsoluteURI;
  static const VXIchar * const SourceEncoding;

  const VXIchar* GetProperty(const VXIchar * key,
                             PropertyLevel L = LAST_PROP) const;
  // Returns property value if a match was found.  Otherwise returns NULL.

  bool SetProperty(const VXIchar * key, const VXIchar * value,
                   PropertyLevel L);

  void SetProperties(const VXMLElement & doc, PropertyLevel level,
                     const VXIMapHolder &);
  // Sets all properties indicated by <property> at this level.

public:
  enum CacheAttrType {
    Audio,
    Document,
    Grammar,
    Object,
    Script,
    Data
  };

  void GetFetchobjCacheAttrs(const VXMLElement & elem,
                             CacheAttrType,
                             VXIMapHolder & fetchobj) const;

public:
  bool GetFetchobjSubmitAttributes(const VXMLElement & elem,
                                   VXIMapHolder & submitData,
                                   VXIMapHolder & fetchobj) const;

  bool GetFetchobjBase(VXIMapHolder & fetchobj) const;
  bool GetFetchobjSubmitType(const VXMLElement & elem,
                             VXIMapHolder & fetchobj) const;
  bool GetFetchobjURIs(const VXMLElement & elem, VXIMapHolder & fetchobj,
                       vxistring & uri, vxistring & fragment) const;

  bool PushProperties(const VXMLElement & elem);
  // Returns: true if any properties were found.

  void PopProperties();
  void PopPropertyLevel(PropertyLevel l);
  void GetProperties(VXIMapHolder &) const;

  PropertyList& operator=(const PropertyList &);
  PropertyList(const PropertyList &);

  PropertyList(const SimpleLogger & l);
  ~PropertyList() { properties.clear(); }

public:
  static bool ConvertTimeToMilliseconds(const SimpleLogger &,
                                        const vxistring & time,
                                        VXIint& result);
  // This function converts an incoming string into an appropriate number of
  // milliseconds.  The format of the string is pretty rigid - some number of
  // digits (0...9) followed by {"s", "ms"} for seconds or milliseconds
  // respectively.  White space or decimals are both illegal.  The result will
  // always contain the 'best guess' even if an error is returned.  Should no
  // units be provided, seconds is assumed and an error is returned.
  //
  // Returns: true  - the time was well formed and result contains number of ms
  //          false - the time was illegal, result contains the best guess

  static bool ConvertValueToFraction(const SimpleLogger &,
                                     const vxistring & value,
                                     VXIflt32& result);
  // This function converts an incoming string into a floating point number
  // in the interval [0, 1].
  //
  // Returns: true  - the number was well formed and in the interval
  //          false - the value was illegal, result contains the best guess

private:
  typedef std::map<vxistring, vxistring> STRINGMAP;
  typedef std::vector<STRINGMAP> PROPERTIES;
  PROPERTIES properties;
  const SimpleLogger & log;
};


inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          VXIint value)
{
  if (m.GetValue() == NULL) return;
  VXIInteger * val = VXIIntegerCreate(value);
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}


inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          const VXIchar * value)
{
  if ((m.GetValue() == NULL) || (value == NULL)) return;
  VXIString * val = VXIStringCreate(value);
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}


inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          const vxistring & value)
{
  if (m.GetValue() == NULL) return;
  VXIString * val = VXIStringCreate(value.c_str());
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}


inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          VXIflt32 value)
{
  if (m.GetValue() == NULL) return;
  VXIFloat * val = VXIFloatCreate(value);
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}

#if (VXI_CURRENT_VERSION >= 0x00030000)
inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          VXIflt64 value)
{
  if (m.GetValue() == NULL) return;
  VXIDouble * val = VXIDoubleCreate(value);
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}

inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          VXIulong value)
{
  if (m.GetValue() == NULL) return;
  VXIULong * val = VXIULongCreate(value);
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}
#endif

inline void AddParamValue(VXIMapHolder & m, const vxistring & name,
                          bool value)
{
  if (m.GetValue() == NULL) return;
  VXIBoolean * val = VXIBooleanCreate((value ? TRUE : FALSE));
  if (val == NULL) throw VXIException::OutOfMemory();
  VXIMapSetProperty(m.GetValue(), name.c_str(),
                    reinterpret_cast<VXIValue *>(val));
}
