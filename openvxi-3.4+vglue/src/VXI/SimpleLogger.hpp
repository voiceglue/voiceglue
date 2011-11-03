#ifndef _VXI_SimpleLogger_HPP
#define _VXI_SimpleLogger_HPP

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

#include <cwchar>
#include <ostream>

extern "C" struct VXIlogInterface;
extern "C" struct VXIMap;
extern "C" struct VXIString;
extern "C" struct VXIVector;
class SimpleLoggerImpl;

class SimpleLogger {
  friend class SimpleLoggerImpl;
public:
  enum InfoTag {
    ENCODING,
    EXCEPTION,
    MESSAGE,
    URI,
    DOC_PARSE_ERROR_DUMPS,
    DOC_DUMPS,
    RECORDING
  };

  static void SetMessageBase(unsigned int base);
  static SimpleLogger * CreateResource(VXIlogInterface *);
  static void DestroyResource(SimpleLogger * &);

public:
  typedef unsigned int TAGID;

  virtual void SetUri(const std::basic_string<wchar_t> & uri) = 0;
  
  // Error logging
  virtual void LogError(int errorNum) const = 0;
  virtual void LogError(int errorNum,
                        InfoTag, const wchar_t * text) const = 0;
  virtual void LogError(int errorNum,
                        InfoTag, const wchar_t * text1,
                        InfoTag, const wchar_t * text2) const = 0;
  virtual void LogError(int errorNum,
                        const wchar_t *key1, const wchar_t * text1) const = 0;

  // Event logging
  virtual void LogEvent(int eventNum, const VXIVector * keys,
                        const VXIVector * values) const = 0;

  // Diagnostic logging.  There are two basic modes.
  virtual bool IsLogging(TAGID tagID) const = 0;
  virtual std::basic_ostream<wchar_t> & StartDiagnostic(TAGID tagID) const = 0;
  virtual void EndDiagnostic() const = 0;
  virtual void LogDiagnostic(TAGID tagID, const wchar_t * text) const = 0;
  virtual void LogDiagnostic(TAGID tagID, const wchar_t* subtag,
                             const wchar_t * format, ...) = 0;

  // Content logging.
  virtual bool LogContent(const wchar_t *mimetype, 
			  const unsigned char *content, 
			  unsigned long size, 
			  VXIString **key, 
			  VXIString **value) const = 0;
  
  // Set module name
  virtual void SetModuleName(const wchar_t* modulename) = 0;

private:
  SimpleLogger() { }
  virtual ~SimpleLogger() { }
};

#endif  /* include guard */
