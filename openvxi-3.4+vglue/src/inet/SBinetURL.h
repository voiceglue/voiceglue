
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

#ifndef __SBINETURL_H_                   /* Allows multiple inclusions */
#define __SBINETURL_H_


#include "VXItypes.h"
#include "VXIvalue.h"
#include "VXIinet.h"

#include "SBinetString.hpp"

#define SB_BOUNDARY "osb_inet_multipart_boundary"
#define SB_MULTIPART "multipart/form-data; boundary=osb_inet_multipart_boundary"
#define SB_URLENCODED "application/x-www-form-urlencoded"
#define CRLF "\r\n"

class SBinetURL
{
public:
  enum Protocol {
    UNKNOWN_PROTOCOL = 0,
    FILE_PROTOCOL = 1,
    HTTP_PROTOCOL = 2,
    HTTPS_PROTOCOL = 3
  };

 public:
  SBinetURL(const SBinetURL& url)
  {
    operator=(url);
  }

  ~SBinetURL()
  {}

  Protocol getProtocol() const
  {
    return _protocol;
  }

  const VXIchar * getAbsolute() const
  {
    return _absoluteURL.c_str();
  }

  const VXIchar * getBase() const
  {
    return _baseURL.c_str();
  }

  const VXIchar * getPath() const
  {
    return _strPath.c_str();
  }

  const VXIchar * getHost() const
  {
    return _host.c_str();
  }

  const char * getNAbsolute() const
  {
    return N_absoluteURL.c_str();
  }

  const char * getNBase() const
  {
    return N_baseURL.c_str();
  }

  const char * getNPath() const
  {
    return N_strPath.c_str();
  }

  const char * getNHost() const
  {
    return N_host.c_str();
  }

  SBinetURL& operator=(const SBinetURL& url);

  bool operator==(const SBinetURL& url);
  bool operator!=(const SBinetURL& url)
  {
    return !operator==(url);
  }

  static VXIinetResult create(const VXIchar* pszName,
                              const VXIchar* pszUrlBase,
                              SBinetURL *& url);

  VXIinetResult parse(const VXIchar* pszName,
                      const VXIchar* pszUrlBase);

  void appendQueryArgsToURL(const VXIMap* _queryArgs);

  VXIString *getContentTypeFromUrl() const;

  SBinetNString queryArgsToNString(const VXIMap* queryArgs) const;

  static bool requiresMultipart(const VXIMap* queryArgs);

  SBinetNString queryArgsToMultipart(const VXIMap* queryArgs);

 public:
  int getPort() const
  {
    return _port;
  }

 private:
  SBinetURL():
    _absoluteURL(), _baseURL(), _host(), _strPath(), _protocol(UNKNOWN_PROTOCOL),
    _port(-1)
  {}

  static SBinetNString valueToNString(const VXIValue* value);

  // Multipart stuff.
  static bool requiresMultipart(const VXIValue* value);
  static bool requiresMultipart(const VXIVector* vxivector);

  static void appendKeyToMultipart(SBinetNString& result, const char *key, bool appendFilename = true);

  static void appendValueToMultipart(SBinetNString& result,
				     const SBinetNString& value);

  static void appendQueryArgsMapToMultipart(SBinetNString& result,
                                            const VXIMap *vximap,
                                            SBinetNString& fieldName);

  static void appendQueryArgsVectorToMultipart(SBinetNString& result,
                                               const VXIVector *vxivector,
                                               SBinetNString& fieldName);

  static void appendQueryArgsToMultipart(SBinetNString& result,
                                         const VXIValue *value,
                                         SBinetNString& fieldName);

 private:
  SBinetString _absoluteURL;
  SBinetString _baseURL;
  SBinetString _host;
  SBinetString _strPath;
  SBinetNString N_absoluteURL;
  SBinetNString N_baseURL;
  SBinetNString N_host;
  SBinetNString N_strPath;
  Protocol _protocol;
  int _port;
};
#endif
