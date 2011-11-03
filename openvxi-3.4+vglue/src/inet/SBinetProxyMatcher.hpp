#ifndef SBINETPROXYMATCHER_HPP
#define SBINETPROXYMATCHER_HPP

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

#include <SWIutilLogger.hpp>
#include <SBinetString.hpp>
#include <VXItypes.h>

class SBinetProxyMatcher
{
  // ................. CONSTRUCTORS, DESTRUCTOR  ............
  //
  // ------------------------------------------------------------
  /**
   * Default constructor.
   **/
 public:
  SBinetProxyMatcher(const char *domain, const char *path,
                     const char *proxyServer, int proxyPort);

  /**
   * Static constructor that parses a rule to generate a proxy matcher.
   **/
 public:
  static SBinetProxyMatcher *createMatcher(const char *rule, const SWIutilLogger *logger);

  /**
   * Destructor.
   **/
 public:
  virtual ~SBinetProxyMatcher();

 public:
  const char *getDomain() const
  {
    return _domain.c_str();
  }

  const char *getPath() const
  {
    return _path.c_str();
  }

  const char *getProxyServer() const
  {
    return _proxyServer.c_str();
  }

  int getProxyPort() const
  {
    return _proxyPort;
  }

  bool matches(const char *domain, const char *path) const;

 private:
  SBinetNString _domain;
  SBinetNString _path;
  SBinetNString _proxyServer;
  int _proxyPort;
};

#endif
