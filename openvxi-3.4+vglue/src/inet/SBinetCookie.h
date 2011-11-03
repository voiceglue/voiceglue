
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

#ifndef __SBCOOKIE_H_                   /* Allows multiple inclusions */
#define __SBCOOKIE_H_

#include "VXItypes.h"
#include <string.h>
#include <cstdlib>
#include <time.h>    // For time( )

#include "SBinetString.hpp"
class SWIutilLogger;
class SBinetURL;

class SBinetCookie
{
public: // CTOR/DTOR
  SBinetCookie(const char* domain,
               const char* path,
               const char* name,
               const char* value,
               const time_t nExpires,
               const bool fSecure ):
    _Name(name), _Value(value), _Domain(domain), _Path(path),
    _nExpires(nExpires), _fSecure(fSecure), _tTimeStamp(0)
  {
    // All values should be text so we can use assignments above.
    // We keep in small char values as this is what is needed most.
    // values are checked for NULL before we are called.
    updateTimeStamp();
  }

  virtual ~SBinetCookie( ) {}

 public:
  void update(const char* value,
              time_t nExpires,
              bool fSecure )
  {
    // All values should be text so we can use strcpy
    // We keep in small char values as this is what is needed most
    // values are checked for NULL before we are called
    _Value = value;
    _nExpires = nExpires;
    _fSecure = fSecure;
    updateTimeStamp();
  }

  bool matchExact(const SBinetCookie *cookie) const;

  bool matchRequest(const char *domain,
                    const char *path,
                    const SWIutilLogger *logger = NULL) const;

  void updateTimeStamp() { _tTimeStamp = time(0); }

  inline const char* getName() const     { return _Name.c_str( ); }
  inline const char* getValue() const    { return _Value.c_str( ); }
  inline const char* getDomain() const   { return _Domain.c_str( ); }
  inline const char* getPath() const    { return _Path.c_str( ); }
  inline time_t getExpires() const { return _nExpires; }
  inline bool getSecure() const { return _fSecure; }
  inline time_t getTimeStamp() const { return _tTimeStamp; }

 public:
  static SBinetCookie *parse(const SBinetURL *url,
                             const char *&cookiespec,
			     const SWIutilLogger *logger);

  // Functions are made public so that they are available to
  // proxy matching code.
  static bool matchDomain(const char *cdomain, const char *domain,
                          const SWIutilLogger* logger = NULL);
  static bool matchPath(const char *cpath, const char *path,
                        const SWIutilLogger *logger = NULL);

 private:
  SBinetCookie():_Name(), _Value(), _Domain(), _Path(), _nExpires(0),
		 _fSecure(false), _tTimeStamp(0)
  {}

  const char *parseAttributes(const char *attributeSpec,
                              const SWIutilLogger *logger);

  struct AttributeInfo;

  typedef void (*AttributeHandler)(AttributeInfo *info,
                                   const char *value,
                                   SBinetCookie *cookie,
                                   const SWIutilLogger *logger);

  struct AttributeInfo
  {
    const char *name;
    bool hasValue;
    AttributeHandler handler;
  };

  static void domainHandler(AttributeInfo *info,
                            const char *value,
                            SBinetCookie *cookie,
                            const SWIutilLogger *logger);
  static void pathHandler(AttributeInfo *info,
                          const char *value,
                          SBinetCookie *cookie,
                          const SWIutilLogger *logger);
  static void expiresHandler(AttributeInfo *info,
                             const char *value,
                             SBinetCookie *cookie,
                             const SWIutilLogger *logger);
  static void secureHandler(AttributeInfo *info,
                            const char *value,
                            SBinetCookie *cookie,
                            const SWIutilLogger *logger);
  static void maxAgeHandler(AttributeInfo *info,
                            const char *value,
                            SBinetCookie *cookie,
                            const SWIutilLogger *logger);

  static AttributeInfo attributeInfoMap[];

  SBinetNString _Name;
  SBinetNString _Value;

  SBinetNString _Domain;
  SBinetNString _Path;
  time_t _nExpires;
  bool _fSecure;
  time_t _tTimeStamp;
};

#endif // include guard
