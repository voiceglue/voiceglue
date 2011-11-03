
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

#ifndef _SB_USE_STD_NAMESPACE
#define _SB_USE_STD_NAMESPACE
#endif

#include "SBinetCookie.h"
#include "SBinetURL.h"
#include "HttpUtils.hpp"
#include "util_date.h"
#include "ap_ctype.h"
#include "SWIutilLogger.hpp"
#include "SBinetLog.h"
#include "SBinetUtils.hpp"

/*****************************************************************************
 *****************************************************************************
 * SBinetCookie Implementation
 *****************************************************************************
 *****************************************************************************
 */

bool SBinetCookie::matchExact(const SBinetCookie *cookie) const
{
  if (strcasecmp(_Name.c_str(), cookie->_Name.c_str())) return(false);
  if (strcasecmp(_Domain.c_str(), cookie->_Domain.c_str())) return(false);
  if (strcasecmp(_Path.c_str(), cookie->_Path.c_str())) return(false);

  return(true);
}

bool SBinetCookie::matchDomain(const char *cdomain,
                               const char *domain,
                               const SWIutilLogger *logger)
{

  bool match = true;

  // Domain name must belong to the domain for the cookie.  The rule is that
  // the cookie's domain must be a suffix of domain.
  int cDomLen = cdomain == NULL ? 0 : strlen(cdomain);
  int domlen = domain == NULL ? 0 : strlen(domain);

  if (cDomLen > domlen)
    match = false;
  else if (cDomLen > 0 &&
           strcasecmp(domain + (domlen - cDomLen), cdomain) != 0)
    match = false;

  if (logger && logger->DiagIsEnabled(MODULE_SBINET_COOKIE_TAGID))
  {
    logger->Diag(MODULE_SBINET_COOKIE_TAGID,
                 L"SBinetCookie::matchDomain",
                 L"cdomain: '%S', domain: '%S', match = '%s'",
                 cdomain, domain, match ? L"true" : L"false");
  }
  return match;
}

bool SBinetCookie::matchPath(const char *cpath,
                             const char *path,
                             const SWIutilLogger *logger)
{
  bool match = true;

  // If the cookie path does not start with a /, then there is no possible
  // match.
  if (!cpath || *cpath != '/')
    match = false;

  // Idem for the path.
  else if (!path || *path != '/')
    match = false;

  else
  {
    const char *p = path + 1;
    const char *cp = cpath + 1;

    // First determine where the first difference is.
    while (*p && *cp &&
           SBinetHttpUtils::toLower(*p) == SBinetHttpUtils::toLower(*cp))
    {
      p++;
      cp++;
    }

    switch (*cp)
    {
     case '\0':
       // The first mismatch is at the end of the cookie-path, if the previous
       // character is a slash, this a match.  Otherwise, we have to make sure
       // that the mismatch char on the path is an anchor, a slash or a query
       // arg delimiter.
       if (*(cp - 1) != '/' && *p && *p != '/' && *p != '?' && *p != '#')
         match = false;
       break;
     case '/':
       // The first mismatch is / on the cookie-path.  If we did not get till
       // the end of the path, the cookie-path is not a prefix of the path.
       // However, if the mismatch character on the path is an anchor or a
       // query argument delimiter, then cookie-path is a prefix of path.
       if (*p && *p != '?' && *p != '#')
         match = false;
       break;
     default:
       match = false;
       break;
    }
  }

  if (logger && logger->DiagIsEnabled(MODULE_SBINET_COOKIE_TAGID))
  {
    logger->Diag(MODULE_SBINET_COOKIE_TAGID, L"SBinetCookie::matchPath",
                 L"cpath: '%S', path: '%S', match = '%s'",
                 cpath, path, match ? L"true" : L"false");
  }
  return match;
}


bool SBinetCookie::matchRequest(const char *domain,
                                const char *path,
                                const SWIutilLogger* logger) const
{
  bool match = true;

  if (!matchDomain(_Domain.c_str(), domain, logger))
  {
    match = false;
  }
  else if (!matchPath(_Path.c_str(), path, logger))
  {
    match = false;
  }

  // TBD, if port was specified by the remote web server for the
  // cookie, the port must be included in the list of ports of the
  // cookie's port attribute. We aren't returning that information yet
  // so we don't even have a port stored in our cookies.

  if (logger && logger->DiagIsEnabled(MODULE_SBINET_COOKIE_TAGID))
  {
    char expStr[64];
    logger->Diag(MODULE_SBINET_COOKIE_TAGID, L"SBinetCookie::matchRequest",
		 L"match: %s, (domain = '%S', path = '%S') compared to "
		 L"(name = '%S', value = '%S', domain = '%S', "
		 L"path = '%S', expires = %S (%d), secure = %s)",
		 (match ? L"true" : L"false"), domain, path, _Name.c_str(),
		 _Value.c_str(), _Domain.c_str(), _Path.c_str(),
		 SBinetUtils::getTimeStampStr(_nExpires, expStr),
		 _nExpires, (_fSecure ? L"true" : L"false"));
  }

  return match;
}

SBinetCookie* SBinetCookie::parse(const SBinetURL *url,
                                  const char *&cookiespec,
                                  const SWIutilLogger *logger)
{
  const char *p = cookiespec;

  for (;;)
  {
    while (*p && ap_isspace(*p)) p++;

    if (!*p)
    {
      cookiespec = p;
      return NULL;
    }

    if (*p == ',')
    {
      // empty header.
      p++;
    }
    else
      break;
  }

  SBinetCookie *cookie = new SBinetCookie();

  cookiespec = p;
  p = SBinetHttpUtils::getToken(p, cookie->_Name);

  if (p == NULL)
  {
    if (logger) logger->Error(252, L"%s%S", L"cookiespec", cookiespec);
    goto failure;
  }

  if ((p = SBinetHttpUtils::expectChar(p,"=")) == NULL || !*p)
  {
    if (logger) logger->Error(253, L"%s%S", L"cookiespec", cookiespec);
    goto failure;
  }

  p++;
  if ((p = SBinetHttpUtils::getCookieValue(p, cookie->_Value)) == NULL)
  {
    if (logger) logger->Error(254, L"%s%S", L"cookiespec", cookiespec);
    goto failure;
  }

  p = SBinetHttpUtils::expectChar(p, ",;");

  if (p == NULL)
  {
    if (logger) logger->Error(255, L"%s%S", L"cookiespec", cookiespec);
    goto failure;
  }

  if (*p)
  {
    p = cookie->parseAttributes(p, logger);
    if (p == NULL)
      goto failure;
  }

  if (cookie->_Domain.length() == 0)
    cookie->_Domain = url->getHost();

  if (cookie->_Path.length() == 0)
  {
    const VXIchar *urlPath = url->getPath();
    const VXIchar *lastSlash = wcsrchr(urlPath, L'/');
    if (!lastSlash || lastSlash == urlPath)
    {
      cookie->_Path = "/";

    }
    else
    {
      cookie->_Path.clear();
      cookie->_Path.append(urlPath, lastSlash - urlPath);
    }
  }

  if (logger && logger->DiagIsEnabled(MODULE_SBINET_COOKIE_TAGID))
  {
    char expStr[64];
    logger->Diag(MODULE_SBINET_COOKIE_TAGID, L"SBinetCookie::parse",
		 L"cookie-spec = '%S', name = '%S', value = '%S', "
		 L"domain = '%S', path = '%S', expires = %S (%d), "
		 L"secure = %s",
		 cookiespec, cookie->_Name.c_str(), cookie->_Value.c_str(),
		 cookie->_Domain.c_str(), cookie->_Path.c_str(),
		 SBinetUtils::getTimeStampStr(cookie->_nExpires, expStr),
		 cookie->_nExpires, (cookie->_fSecure ? L"true" : L"false"));
  }

  cookiespec = p;

  return cookie;

 failure:
  delete cookie;
  cookiespec = NULL;
  return NULL;
}

SBinetCookie::AttributeInfo SBinetCookie::attributeInfoMap[] = {
  { "path", true, pathHandler },
  { "domain", true, domainHandler },
  { "expires", true, expiresHandler },
  { "secure", false, secureHandler },
  { "max-age", true, maxAgeHandler }
};

void SBinetCookie::domainHandler(AttributeInfo *info, const char *value,
                                 SBinetCookie *cookie,
                                 const SWIutilLogger *logger)
{
  cookie->_Domain = value;
}

void SBinetCookie::pathHandler(AttributeInfo *info, const char *value,
                               SBinetCookie *cookie,
                               const SWIutilLogger *logger)
{
  cookie->_Path = value;

  // remove trailing slash if not the only character in the path.
  int len = cookie->_Path.length();

  if (len == 0)
    cookie->_Path = "/";
  else if (--len > 0 && cookie->_Path[len] == '/')
    cookie->_Path.resize(len);
}

void SBinetCookie::expiresHandler(AttributeInfo *info, const char *value,
                                  SBinetCookie *cookie,
                                  const SWIutilLogger *logger)
{
  cookie->_nExpires = ap_parseHTTPdate(value);
}

void SBinetCookie::secureHandler(AttributeInfo *info, const char *value,
                                 SBinetCookie *cookie,
                                 const SWIutilLogger *logger)
{
  cookie->_fSecure = true;
}

void SBinetCookie::maxAgeHandler(AttributeInfo *info, const char *value,
                                 SBinetCookie *cookie,
                                 const SWIutilLogger *logger)
{
  cookie->_nExpires = time(NULL) + atoi(value);
}

const char *SBinetCookie::parseAttributes(const char *attributeSpec,
                                          const SWIutilLogger *logger)
{
  const char *p = attributeSpec;
  for (;;)
  {
    while (*p && ap_isspace(*p)) p++;

    // end of cookie string.
    if (!*p || *p == ',') break;

    if (*p == ';')
    {
      // empty attribute.
      p++;
      continue;
    }

    attributeSpec = p;

    SBinetNString attributeNStr;

    if ((p = SBinetHttpUtils::getToken(p, attributeNStr)) == NULL)
    {
      if (logger)
        logger->Error(256, L"%s%S", L"attributeSpec", attributeSpec);
      return NULL;
    }

    const char *attribute = attributeNStr.c_str();
    AttributeInfo *info = NULL;

    for (int i = (sizeof (attributeInfoMap)) / (sizeof (attributeInfoMap[0])) - 1; i >= 0; i--)
    {
      if (strcasecmp(attributeInfoMap[i].name, attribute) == 0)
      {
        info = &attributeInfoMap[i];
        break;
      }
    }

    // If we haven't found a matching attributeInfo, we assume that the
    // attribute has a value.
    bool hasValue = info != NULL ? info->hasValue : true;
    SBinetNString valueNStr;
    const char *value = NULL;

    if (hasValue)
    {
      // Now we have to deal with attributes of the form x=y
      if ((p = SBinetHttpUtils::expectChar(p,"=")) == NULL || !*p)
      {
        if (logger)
          logger->Error(257, L"%s%S%s%S", L"attributeSpec", attributeSpec,
                        L"attribute", attribute);
        return NULL;
      }

      p++;
      if ((p = SBinetHttpUtils::getCookieValue(p, valueNStr)) == NULL)
      {
        if (logger)
          logger->Error(258, L"%s%S%s%S", L"attributeSpec", attributeSpec,
                        L"attribute", attribute);
        return NULL;
      }
      p = SBinetHttpUtils::expectChar(p, ",;");

      value = valueNStr.c_str();

      if (p == NULL)
      {
        if (logger)
          logger->Error(259, L"%s%S%s%S%s%S",
                        L"attributeSpec", attributeSpec,
                        L"attribute", attribute, L"value", value);
        return NULL;
      }
      if (logger)
        logger->Diag(MODULE_SBINET_COOKIE_TAGID,
                     L"SBinetCookie::parseAttributes",
                     L"attribute = <%S>. value = <%S>", attribute, value);
    }
    else
    {
      p = SBinetHttpUtils::expectChar(p, ",;");

      if (p == NULL)
      {
        if (logger)
          logger->Error(260, L"%s%S%s%S",
                        L"attributeSpec", attributeSpec,
                        L"attribute", attribute);
        return NULL;
      }
      if (*p && *p != ',' && *p != ';' && logger)
      {
        logger->Error(261, L"%s%S%s%S", L"attributeSpec", attributeSpec,
                      L"attribute", attribute);
      }
    }
    if (info != NULL)
    {
      info->handler(info, value, this, logger);
    }
  }

  return p;
}
