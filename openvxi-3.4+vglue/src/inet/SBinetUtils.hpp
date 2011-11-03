
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

#ifndef SBINETUTILS_HPP
#define SBINETUTILS_HPP

#include "VXIvalue.h"

class SBinetValidator;

class SBinetMD5 {
public:
  SBinetMD5();
  SBinetMD5(const VXIchar* url);
  SBinetMD5(const SBinetMD5 & x);
  SBinetMD5 & operator=(const SBinetMD5 & x);
  
  void Clear();
  bool IsClear() const;
  bool GenMD5Key(const VXIchar* content);
  unsigned char raw[16];   
};

class SBinetUtils
{
  // Simple helper functions to retrieve information from a VXIMap.
 public:
  static bool getInteger(const VXIMap *theMap, const VXIchar *prop, VXIint32& value);

  static bool getFloat(const VXIMap *theMap, const VXIchar *prop, VXIflt32& value);

  static const VXIchar *getString(const VXIMap *theMap, const VXIchar *prop);

  static bool getContent(const VXIMap *theMap, const VXIchar *prop,
                         const char* &content, VXIulong& contentLength);

  static VXIContent *createContent(const char *str);

  static char *getTimeStampStr(time_t  timestamp,
			       char   *timestampStr);

  static char *getTimeStampMsecStr(char  *timestampStr);

  static bool setValidatorInfo(VXIMap *streamInfo, const SBinetValidator *validator);

 private:
  SBinetUtils();
};


#endif
