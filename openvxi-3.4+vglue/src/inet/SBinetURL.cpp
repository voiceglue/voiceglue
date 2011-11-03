
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

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifdef _WIN32
#undef HTTP_VERSION
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#else
#include <unistd.h> // for getcwd()
#endif /* WIN32 */

#include <stdio.h>
#include <string.h>

#include "VXIvalue.h"
#include "VXIinet.h"
#include "VXItrd.h"

#include "SBinetURL.h"
#include "SBinetChannel.h"
#include "HttpUtils.hpp"

#include <SBinetString.hpp>

#define INET_MAX_PATH 1024

#ifdef _WIN32
#include <time.h>
struct tm* localtime_r(const time_t* t, struct tm* dummy)
{
  return localtime(t);
}
#endif

// #if defined(_decunix_) || defined(_solaris_)
// static int my_wcscasecmp(const wchar_t *s1, const wchar_t *s2)
// {
//   register unsigned int u1, u2;

//   for (;;) {
//     u1 = (unsigned int) *s1++;
//     u2 = (unsigned int) *s2++;
//     if (HttpUtils::toUpper(u1) != HttpUtils::toUpper(u2)) {
//       return HttpUtils::toUpper(u1) - HttpUtils::toUpper(u2);
//     }
//     if (u1 == '\0') {
//       return 0;
//     }
//   }
// }
// #else
// #define my_wcscasecmp ::wcscasecmp
// #endif

static void
appendArrayIndexToName(SBinetNString& fieldName,
                       VXIunsigned index)
{
  char tempBuf[8];

  sprintf(tempBuf, ".%d", index);
  fieldName += tempBuf;
}

VXIinetResult SBinetURL::create(const VXIchar *pszName,
                                const VXIchar *pszUrlBase,
                                SBinetURL *& url)
{
  url = new SBinetURL();

  if (url == NULL)
  {
    return VXIinet_RESULT_OUT_OF_MEMORY;
  }

  VXIinetResult rc = url->parse(pszName, pszUrlBase);
  if (rc != VXIinet_RESULT_SUCCESS)
  {
    delete url;
    url = NULL;
  }
  return rc;
}

SBinetURL& SBinetURL::operator=(const SBinetURL& rhs)
{
  if (this != &rhs)
  {
    _absoluteURL = rhs._absoluteURL;
    _baseURL = rhs._baseURL;
    _host = rhs._host;
    _strPath = rhs._strPath;
    _protocol = rhs._protocol;
    _port = rhs._port;
    N_absoluteURL = rhs.N_absoluteURL;
    N_baseURL = rhs.N_baseURL;
    N_host = rhs.N_host;
    N_strPath = rhs.N_strPath;
  }
  return *this;
}

bool SBinetURL::operator==(const SBinetURL& rhs)
{
  if (this == &rhs)
    return true;

  if (_protocol != rhs._protocol)
    return false;

  if (::wcscmp(_strPath.c_str(), rhs._strPath.c_str()) != 0)
    return false;

  if (_protocol >= HTTP_PROTOCOL)
  {
    if ( _port != rhs._port)
      return false;

    if (SBinetHttpUtils::casecmp(_host.c_str(), rhs._host.c_str()) != 0)
      return false;
  }

  return true;
}

#ifdef _WIN32
VXIinetResult SBinetURL::parse(const VXIchar* pszUrl, const VXIchar* pszUrlBase)
{
  VXIinetResult eResult( VXIinet_RESULT_SUCCESS );

  if( !pszUrl || !*pszUrl)
  {
    //Error(200, L"%s%s", L"Operation", L"parse URL");
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  if (pszUrlBase != NULL)
    _baseURL = pszUrlBase;
  else
    _baseURL = L"";

  // If the caller is using "file:", just strip it because apparently
  // Internet*() doesn't do the right thing with file URI's
  bool relativeFileUri = false;
  if (!wcsncmp(L"file:", pszUrl, 5)) { // file:...
    pszUrl += 5;
    if (!wcsncmp(L"//", pszUrl, 2)) // file://...
      pszUrl += 2;
    relativeFileUri = true;
  }

  wchar_t *tmpUrl = NULL;
  const wchar_t *absoluteUrl = pszUrl;

  // The parsing of URL is complicated by the fact that we want unsafe
  // characters to be encoded using %-notation for HTTP URLs, but we want
  // %-notatation to be converted into unsaface characters for file URLs.  So,
  // on entry we convert the URL into unsafe notation, then for HTTP URLs, we
  // re-encode it back using %-notation.

  // Combine the base and (possibly relative) URL.  Since we stripped the
  // transport type above, don't combine them if the transport type of the
  // base URI differs.
  if( pszUrlBase && pszUrlBase[0] && !(relativeFileUri && (0 == wcsncmp(L"http:", pszUrlBase, 5))))
  {
    VXIulong len = (::wcslen(pszUrlBase) + ::wcslen(pszUrl)) * 3;
    tmpUrl = new VXIchar [len];
    if( !tmpUrl )
    {
      //Error(103, NULL);
      return VXIinet_RESULT_OUT_OF_MEMORY;
    }

    if (InternetCombineUrl(pszUrlBase, pszUrl, tmpUrl, &len, ICU_BROWSER_MODE | ICU_DECODE| ICU_NO_ENCODE) == TRUE)
    {
      absoluteUrl = tmpUrl;
    }
    else
    {
      int errCode = GetLastError();
      //Error(225, NULL);
      delete [] tmpUrl;
      return VXIinet_RESULT_NON_FATAL_ERROR;
    }
  }
  else
  {
    VXIulong len = ::wcslen (pszUrl) * 3;
    tmpUrl = new VXIchar [len];
    if( !tmpUrl )
    {
      //Error(103, NULL);
      return VXIinet_RESULT_OUT_OF_MEMORY;
    }
    if (InternetCanonicalizeUrl(pszUrl, tmpUrl, &len, ICU_BROWSER_MODE | ICU_DECODE | ICU_NO_ENCODE) == TRUE)
    {
      absoluteUrl = tmpUrl;
    }
    else
    {
      int errCode = GetLastError();
      //Error(225, NULL);
      delete [] tmpUrl;
      return VXIinet_RESULT_NON_FATAL_ERROR;
    }
  }

  // Now parse the absolute URL to decide if it is file or network access
  int pathLen = 0;
  int queryLen = 0;
  wchar_t* queryPtr = ::wcschr(absoluteUrl, L'?');
  if (queryPtr)
  {
    pathLen = queryPtr - absoluteUrl + 1;
    queryLen = ::wcslen(queryPtr) + 1;
  }
  else
  {
    pathLen = ::wcslen(absoluteUrl) + 1;
    queryLen = 0;
  }

  if (pathLen < INET_MAX_PATH)
    pathLen = INET_MAX_PATH;

  wchar_t protocol[INET_MAX_PATH];
  wchar_t* host = new wchar_t [pathLen];
  wchar_t* urlPath = new wchar_t [pathLen];
  wchar_t* query = NULL;
  if (queryLen > 0)
    query = new wchar_t [queryLen];

  URL_COMPONENTS components;
  memset (&components, 0, sizeof (URL_COMPONENTS));
  components.dwStructSize = sizeof (URL_COMPONENTS);
  components.lpszScheme = protocol;
  components.dwSchemeLength = INET_MAX_PATH;
  components.lpszHostName = host;
  components.dwHostNameLength = pathLen;
  components.lpszUrlPath = urlPath;
  components.dwUrlPathLength = pathLen;
  components.lpszExtraInfo = query;
  components.dwExtraInfoLength = queryLen;

  if(InternetCrackUrl(absoluteUrl, ::wcslen (absoluteUrl), 0, &components) == TRUE)
  {
    switch (components.nScheme)
    {
     case INTERNET_SCHEME_FILE:
       // File access, return the local file path
       _protocol = FILE_PROTOCOL;
       _strPath = urlPath;
       _absoluteURL = absoluteUrl;
       break;
     case INTERNET_SCHEME_HTTPS:
     case INTERNET_SCHEME_HTTP:
       {
         // HTTP access, return the absolute URL
         _protocol = (components.nScheme == INTERNET_SCHEME_HTTP ?
                    HTTP_PROTOCOL : HTTPS_PROTOCOL);

         _absoluteURL = absoluteUrl;

         // remove trailing / in absolute URL to ensure that www.spechworks.com
         // and www.speechwork.com/ are seen as the same URL.
         int idx = _absoluteURL.length() - 1;
         if (_absoluteURL[idx] == L'/')
         {
           _absoluteURL.resize(idx);
         }

         _host = host;

         // Retrieve the host name, the port number and the local file.
         _port = components.nPort;

         if (components.dwUrlPathLength == 0)
         {
           _strPath = L"/";
         }
         else
         {
           _strPath = urlPath;
           SBinetNString utf8path, escapedPath;
           SBinetHttpUtils::utf8encode(urlPath, utf8path);
           SBinetHttpUtils::escapeString(utf8path.c_str(),
                                         SBinetHttpUtils::URL_PATH,
                                         escapedPath);
           _strPath = escapedPath;
         }

         if (components.dwExtraInfoLength > 0)
           _strPath += query;
         break;
       }
     default:
       delete [] tmpUrl;
       delete [] host;
       delete [] urlPath;
       delete [] query;
       return VXIinet_RESULT_INVALID_ARGUMENT;
    }
  }
  else
  {
    // Couldn't be parsed.

    // If the absoluteUrl contains a colon, it is because the parsing of the
    // URL failed.  If the URL contained a colon and were valid, it would have
    // been parsed sucessfully by InternetCrackURL.  If it doesn't contain a
    // colon, we assume it is a path relative to the current directory.
    //
    if (::wcschr(absoluteUrl, L':') != NULL)
    {
      delete [] tmpUrl;
      delete [] host;
      delete [] urlPath;
      delete [] query;
      return VXIinet_RESULT_INVALID_ARGUMENT;
    }

    wchar_t *ignored;
    *urlPath = L'\0';
    _protocol = FILE_PROTOCOL;

    if ((GetFullPathName(absoluteUrl, pathLen, urlPath, &ignored) > 0) &&
       *urlPath)
    {
      _strPath = urlPath;
      _absoluteURL = absoluteUrl;
    }
    else
    {
      //Error(225, L"%s%s", L"URL", pszUrl);
      eResult = VXIinet_RESULT_INVALID_ARGUMENT;
    }
  }

  N_absoluteURL = _absoluteURL;
  N_baseURL = _baseURL;
  N_host = _host;
  N_strPath = _strPath;

  delete [] tmpUrl;
  delete [] host;
  delete [] urlPath;
  delete [] query;

  return eResult;
}

#else /* not WIN32 */

struct URLInfo
{
  SBinetString protocol;
  SBinetString fragment;
  SBinetString query;
  SBinetString path;
  SBinetString host;
  int port;
};

typedef std::basic_string<VXIchar> vxistring;

static VXIinetResult canonicalizeUrl(const VXIchar* const srcUrl, VXIchar* finalUrl)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS; 
  vxistring url = srcUrl, tmp;
  
  if( !finalUrl ) return VXIinet_RESULT_INVALID_ARGUMENT; 
  // make sure we got a copy
  wcscpy(finalUrl, srcUrl);
  
  // TODO: need to convert to lowercase ???
  
  // return if a protocol not found
  if( url.find(L"://") == vxistring::npos ) 
    return VXIinet_RESULT_SUCCESS;      
   
  // remove "../" and adjust path appropriately
  vxistring::size_type idx, i, queryIndex;
  idx = url.find(L"../", 0);
  while( idx != vxistring::npos ) {    
    // don't want to touch any part after ? (query string)
    queryIndex = url.find(L"?");
    if( queryIndex != vxistring::npos && queryIndex < idx ) break;
    
    // part contains ../
    tmp = url.substr(idx);  
    // part contains base <protocol>:// where <protocol> could be
    // http, https, ftp and so on
    url.erase(idx-1);       
    tmp.erase(0, 3);  // remove ../
    i = url.rfind(L"/");
    if( i != vxistring::npos &&
        url.substr(0,i-1).find(L"://") != vxistring::npos ) {
      url.erase(i);  // adjust base, we must keep the root i.e: http://abc.com
    }
    if( url[url.length()-1] != L'/' ) url += L"/";
    url += tmp;
    idx = url.find(L"../", 0);  // continue search for ../
  }
  // Make a copy of final result
  wcscpy(finalUrl, url.c_str());      
  return rc;  
}

static VXIinetResult parseURL(const VXIchar* const url, URLInfo& urlInfo)
{
  VXIinetResult rc = VXIinet_RESULT_SUCCESS;
  // Initialize the URLInfo structure.
  urlInfo.port = -1;
  
  // Check to see if the URL is invalid.
  if (!url || (url[0] == 0))
    return VXIinet_RESULT_NON_FATAL_ERROR;

  VXIchar* tmpUrl = new VXIchar [wcslen(url) + 1];
  if( !tmpUrl ) return VXIinet_RESULT_OUT_OF_MEMORY;
  wcscpy(tmpUrl, url);
  const VXIchar* tmpUrlOriginal = tmpUrl;

  // Check for the protocol part.
  bool needToCanonicalize = false;
  VXIchar* protocolEndPtr = wcschr(tmpUrl, L':');
  if (protocolEndPtr && (!wcsncmp(tmpUrl, L"file", 4) || !wcsncmp(tmpUrl, L"http", 4)))
  {
    // Found the protocol.  Copy it to the URLInfo structure.
    int protLen = protocolEndPtr - tmpUrl;
    urlInfo.protocol.append(tmpUrl, protLen);

    // Advance past the protocol part of the URL.
    if (0 == wcsncmp(protocolEndPtr + 1, L"//", 2))
    {
      tmpUrl = protocolEndPtr + 3;
    }
    else
    {
      tmpUrl = protocolEndPtr + 1;
      needToCanonicalize = 1;
    }
  }
  else
  {
    // The default protocol is the file protocol.
    urlInfo.protocol = L"file";
  }

  if (!wcscmp(urlInfo.protocol.c_str(), L"file"))
  {
    // If the URL uses the file protocol, the rest of the URL is the path.

    // The path needs to be canonicalized.  It could still be an absolute
    // path if it starts with a '/'.  If it does not, create the absolute
    // path by prepending the current path with the current directory.
    if (needToCanonicalize && tmpUrl[0] != '/')
    {
      char buf[1024];
      urlInfo.path = getcwd(buf, 1024);
      urlInfo.path += '/';
    }
    VXIchar* endPtr = wcschr(tmpUrl, L'?');
    VXIchar* fragmentPtr = wcschr(tmpUrl, L'#');
    if (endPtr)
    {
      if (fragmentPtr && (fragmentPtr < endPtr))
        endPtr = fragmentPtr;
    }
    else if (fragmentPtr)
    {
      endPtr = fragmentPtr;
    }

    // Strip the query and/or fragment part of the file URI if
    // there is any.
    if (endPtr)
      *endPtr = L'\0';

    urlInfo.path += tmpUrl;
  }
  else
  {
    // Check for the fragment part.
    VXIchar* fragmentPtr = wcsrchr(tmpUrl, L'#');
    if (fragmentPtr)
    {
      // Found the fragment.  Copy it to the URLInfo structure.
      *fragmentPtr = L'\0';
      urlInfo.fragment = ++fragmentPtr;
    }

    // Check for the query part.
    VXIchar* queryPtr = wcsrchr(tmpUrl, L'?');
    if (queryPtr)
    {
      // Found the query.  Copy it to the URLInfo structure.
      *queryPtr = L'\0';
      urlInfo.query = ++queryPtr;
    }

    // Check for the path part.
    VXIchar* pathPtr = wcschr(tmpUrl, L'/');
    if (pathPtr)
    {
      // Found the path.  Copy it to the URLInfo structure.
      // First, we need to UTF-8 encode it and then escape it.
      SBinetNString utfPath, escapedPath;
      SBinetHttpUtils::utf8encode(pathPtr, utfPath);
      SBinetHttpUtils::escapeString(utfPath.c_str(),
                                    SBinetHttpUtils::URL_PATH,
                                    escapedPath);

      urlInfo.path = escapedPath;

      // Append the query to the path.
      if (queryPtr)
      {
        // Add the query to the path.
        urlInfo.path += L'?';
        urlInfo.path += urlInfo.query;
      }

      *pathPtr = L'\0';
    }
    else
    {
      // No path was specified so set the path to root ("/").
      urlInfo.path = "/";
    }

    // Check for the username:password part.
    VXIchar* userpassEndPtr = wcschr(tmpUrl, L'@');
    if (userpassEndPtr)
    {
      // For now, just ignore username and password.
      tmpUrl = userpassEndPtr + 1;
    }

    // Check for the port part.
    VXIchar* portPtr = wcsrchr(tmpUrl, L':');
    if (portPtr)
    {
      *portPtr = L'\0';
      ++portPtr;

      // Check if the port is valid (all digits).
      VXIchar* end = NULL;
      long port = wcstol(portPtr, &end, 10);

      if ((end) && (*end == L'\0'))
        urlInfo.port = (int) port;
    }

    // The rest is the host part.
    urlInfo.host = tmpUrl;

    if (urlInfo.host[0] == 0)
    {
      delete [] tmpUrlOriginal;
      return VXIinet_RESULT_NON_FATAL_ERROR;
    }
  }

  delete [] tmpUrlOriginal;

  return VXIinet_RESULT_SUCCESS;
}

static VXIinetResult combineURL(const VXIchar* const baseUrl,
                                const VXIchar* const relativeUrl,
                                SBinetString& absoluteUrl)
{
  // Check to see if the relative URL is referencing another host
  //    (i.e. the protocol is specified).
  const VXIchar* protocolEndPtr = wcschr(relativeUrl, L':');
  if (!baseUrl || (protocolEndPtr && (!wcsncmp(relativeUrl, L"file", 4) || !wcsncmp(relativeUrl, L"http", 4))))
  {
    // HACK to allow OSR to load absolute local URLs
    if (wcsstr(relativeUrl, L"/file://"))
      absoluteUrl = relativeUrl + 1;
    else
      absoluteUrl = relativeUrl;
  }
  else
  {
    absoluteUrl = baseUrl;

    const VXIchar* absoluteUrlStr = absoluteUrl.c_str();
    const VXIchar* pathPtr = wcsstr(absoluteUrlStr, L"://");
    if (pathPtr)
      pathPtr += 3;
    else
      pathPtr = absoluteUrlStr;

    if (relativeUrl[0] == L'/' && relativeUrl[1] != L'/')
    {
      // The relative URL is an absolute path.

      // Find the pointer to the beginning of the path in the absoluteUrl.
      pathPtr = wcschr(pathPtr, L'/');
      if (pathPtr)
        absoluteUrl.resize(pathPtr - absoluteUrlStr);
    }
    else
    {
      // The relative URL is a relative path.

      // Find the pointer to the end of the path.
      pathPtr = wcsrchr(pathPtr, L'/');
      if (pathPtr)
      {
        // Clear anything after the last slash in the path part of the
        //    absolute URL.
        ++pathPtr;
        absoluteUrl.resize(pathPtr - absoluteUrlStr);
      }
      else
      {
        // Add a slash at the end of the absolute URL if it didn't have one.
        absoluteUrl += L"/";
      }
    }
    absoluteUrl += relativeUrl;
  }
  
  // canonicalize abs. url
  VXIchar* tmpUrl = new VXIchar[absoluteUrl.length()+1];  
  if( !tmpUrl ) return VXIinet_RESULT_OUT_OF_MEMORY;
  VXIinetResult rc = canonicalizeUrl(absoluteUrl.c_str(), tmpUrl);
  if( rc != VXIinet_RESULT_SUCCESS ) {
    delete [] tmpUrl;
    return rc;
  }
  absoluteUrl = tmpUrl;
  delete [] tmpUrl;
  return VXIinet_RESULT_SUCCESS;
}

VXIinetResult
SBinetURL::parse(const VXIchar* pszUrl,
                 const VXIchar* pszUrlBase)
{
  // TBD: Must apply SPR 7530 fix here too, for now only in WinInetResolveUrl,
  // support for relative URL starting with / as specified by the RFC that
  // defines file:// access
  VXIinetResult eResult( VXIinet_RESULT_SUCCESS );

  if( !pszUrl || !pszUrl[0])
  {
    //Error(200, L"%s%s", L"Operation", L"parse URL");
    return VXIinet_RESULT_INVALID_ARGUMENT;
  }

  if (pszUrlBase != NULL)
    _baseURL = pszUrlBase;
  else
    _baseURL = L"";

  // Combine the base and relative URLs to get an absolute URL.
  eResult = combineURL(pszUrlBase, pszUrl, _absoluteURL);

  if (eResult == VXIinet_RESULT_SUCCESS)
  {
    // Parse the absolute URL into its components.
    URLInfo urlInfo;
    eResult = parseURL(_absoluteURL.c_str(), urlInfo);

    if (eResult == VXIinet_RESULT_SUCCESS)
    {
      if (!wcscmp(urlInfo.protocol.c_str(), L"file"))
      {
        _protocol = FILE_PROTOCOL;
      }
      else if (!wcscmp(urlInfo.protocol.c_str(), L"http"))
      {
        _protocol = HTTP_PROTOCOL;
        _host = urlInfo.host;
        _port = urlInfo.port == -1 ? 80 : urlInfo.port;
      }
      else if (!wcscmp(urlInfo.protocol.c_str(), L"https"))
      {
        _protocol = HTTPS_PROTOCOL;
        _host = urlInfo.host;
        _port = urlInfo.port == -1 ? 443 : urlInfo.port;
      }
      else
      {
        eResult = VXIinet_RESULT_NON_FATAL_ERROR;
      }
    }

    if (eResult == VXIinet_RESULT_SUCCESS)
    {
      _strPath = urlInfo.path;

      // Remove trailing / in absolute URL to ensure that www.vocalocity.com
      // and www.vocalocity.com/ are seen as the same URL.
      int idx = _absoluteURL.length() - 1;
      if (_absoluteURL[idx] == L'/')
        _absoluteURL.resize(idx);
    }
  }

  N_absoluteURL = _absoluteURL;
  N_baseURL = _baseURL;
  N_host = _host;
  N_strPath = _strPath;

  return eResult;
}

#endif /* WIN32 */



SBinetNString SBinetURL::valueToNString(const VXIValue* value)
{
  // Convert numeric types using a narrow character buffer in order to
  // avoid the need for swprintf( ) which doesn't exist in the GNU
  // GCC C library for GCC 2.x and earlier.
  char tempBuf[32];
  *tempBuf = '\0';
  switch (VXIValueGetType(value))
  {
   case VALUE_BOOLEAN:
     {
       VXIbool valBool = VXIBooleanValue( (const VXIBoolean *)value );
       sprintf (tempBuf, "%s", valBool ? "true" : "false");
     }
     break;
   case VALUE_INTEGER:
     {
       VXIint32 valInt = VXIIntegerValue( (const VXIInteger *)value );
       sprintf (tempBuf, "%d", valInt);
     }
     break;
   case VALUE_FLOAT:
     {
       VXIflt32 valFloat = VXIFloatValue( (const VXIFloat *)value );
       sprintf (tempBuf, "%f", valFloat);
     }
     break;
#if (VXI_CURRENT_VERSION >= 0x00030000)
   case VALUE_ULONG:
     {
       VXIulong valInt = VXIULongValue( (const VXIULong *)value );
       sprintf (tempBuf, "%u", (unsigned int) valInt);
     }
     break;
   case VALUE_DOUBLE:
     {
       VXIflt64 valFloat = VXIDoubleValue( (const VXIDouble *)value );
       sprintf (tempBuf, "%f", valFloat);
     }
     break;
#endif
   case VALUE_STRING:
       return VXIStringCStr( (const VXIString *)value );
   case VALUE_PTR:
     {
       void *valPtr = VXIPtrValue( (const VXIPtr *)value );
       sprintf (tempBuf, "%p", valPtr);
     }
     break;
   case VALUE_MAP:
   case VALUE_VECTOR:
   case VALUE_CONTENT:
   default:
     // These types are supposed to be handled before entering this function.
     break;
  }

  return tempBuf;
}


bool SBinetURL::requiresMultipart(const VXIValue *value)
{
  switch (VXIValueGetType(value))
  {
   case VALUE_CONTENT:
     return true;
   case VALUE_MAP:
     return requiresMultipart((const VXIMap*) value);
   case VALUE_VECTOR:
     return requiresMultipart((const VXIVector*) value);
   default:
     return false;
  }
}


bool SBinetURL::requiresMultipart(const VXIMap* vximap)
{
  if (vximap == NULL) return false;

  const VXIchar  *key = NULL;
  const VXIValue *value = NULL;
  bool result = false;

  VXIMapIterator *mapIterator = VXIMapGetFirstProperty( vximap, &key, &value );
  do
  {
    if (key != NULL && value != NULL && requiresMultipart(value))
    {
      result = true;
      break;
    }
  }
  while (VXIMapGetNextProperty(mapIterator, &key, &value) == VXIvalue_RESULT_SUCCESS);

  VXIMapIteratorDestroy(&mapIterator);
  return result;
}

bool SBinetURL::requiresMultipart(const VXIVector* vxivector)
{
  for (VXIunsigned i = VXIVectorLength(vxivector); i > 0; i--)
  {
    const VXIValue* value = VXIVectorGetElement(vxivector, i);
    if (value != NULL && requiresMultipart(value))
    {
      return true;
    }
  }
  return false;
}


void SBinetURL::appendQueryArgsToURL(const VXIMap* queryArgs)
{
  SBinetNString queryArgsNStr = queryArgsToNString(queryArgs);
  if (queryArgsNStr.length() > 0)
  {
    const wchar_t delim = wcschr(_strPath.c_str(), L'?') == NULL ? L'?' : L'&';

    _strPath += delim;
    _strPath += queryArgsNStr;

    _absoluteURL += delim;
    _absoluteURL += queryArgsNStr;

    // maintain narrow and wide representations synchronized.
    N_strPath += delim;
    N_strPath += queryArgsNStr;
    N_absoluteURL += delim;
    N_absoluteURL += queryArgsNStr;
  }
}


SBinetNString
SBinetURL::queryArgsToNString(const VXIMap* queryArgs) const
{
  SBinetNString result;

  if (queryArgs)
  {
    VXIString *strResult = VXIValueToString((const VXIValue *) queryArgs, L"",
					    VALUE_FORMAT_URL_QUERY_ARGS);
    if (strResult)
    {
      // URL query args encoding ensures we only have ASCII here
      result += VXIStringCStr(strResult);
      VXIStringDestroy(&strResult);
    }
  }

  return result;
}


/*
 * Utilities
 */
void SBinetURL::appendKeyToMultipart(SBinetNString& result, const char *key, bool appendFilename )
{
  // Print the boundary start
  if (result.length() != 0)
    result += CRLF;

  result += "--" SB_BOUNDARY CRLF;

  result += "Content-Disposition: form-data; name=\"";
  result += key;
  result += "\"";
  
  if (appendFilename){
     char filename[128];
     struct tm* ct;
     struct tm ct_r = {0};
     time_t t = time(NULL);

     ct = localtime_r(&t, &ct_r);
     sprintf(filename, "%04d%02d%02d%02d%02d%02d.ulaw", ct->tm_year+1900,
       ct->tm_mon+1, ct->tm_mday, ct->tm_hour, ct->tm_min,
       ct->tm_sec);

     result += "; filename=\"";
     result += filename;
     result += "\"";
  }

  result += CRLF;
}


void SBinetURL::appendValueToMultipart(SBinetNString& result, const SBinetNString& value)
{
//   char tempbuf[20];

//   if (value.length() > 0)
//   {
//     result += "Content-Type: text/plain" CRLF;

//     result += "Content-Length: ";
//     sprintf(tempbuf, "%d", value.length());
//     result += tempbuf;
//     result += CRLF;

//     // blank line.
//     result += CRLF;

//     result += value;
//   }
  result += CRLF;
  result += value;
}


void SBinetURL::appendQueryArgsVectorToMultipart(SBinetNString& result,
                                                 const VXIVector *vxivector,
                                                 SBinetNString& fieldName)
{
  const VXIValue *value = NULL;
  VXIunsigned vectorLen = VXIVectorLength(vxivector);
  int prevLen = fieldName.length();

  for(VXIunsigned i = 0 ; i < vectorLen ; i++)
  {
    value = VXIVectorGetElement(vxivector, i);
    if (value != NULL)
    {
      appendArrayIndexToName(fieldName, i);

      appendQueryArgsToMultipart(result, value, fieldName);

      fieldName.resize(prevLen);
    }
  }
}


void SBinetURL::appendQueryArgsMapToMultipart(SBinetNString& result,
                                              const VXIMap *vximap,
                                              SBinetNString& fieldName)
{
  const VXIchar  *key = NULL;
  const VXIValue *value = NULL;
  int prevLen = fieldName.length();

  VXIMapIterator *mapIterator = VXIMapGetFirstProperty(vximap, &key, &value );
  do
  {
    if (key != NULL && value != NULL)
    {
      fieldName += '.';
      fieldName += key;

      appendQueryArgsToMultipart(result, value, fieldName);

      fieldName.resize(prevLen);
    }
  }
  while (VXIMapGetNextProperty(mapIterator, &key, &value) ==
         VXIvalue_RESULT_SUCCESS);

  VXIMapIteratorDestroy(&mapIterator);
}


SBinetNString SBinetURL::queryArgsToMultipart(const VXIMap* queryArgs)
{
  SBinetNString result;

  if(!queryArgs) return(result);

  const VXIchar *key = NULL;
  const VXIValue *value = NULL;

  VXIMapIterator* mapIterator = VXIMapGetFirstProperty( queryArgs, &key, &value );

  do
  {
    if (key != NULL && value != NULL)
    {
      SBinetNString fieldName = key;
      appendQueryArgsToMultipart(result, value, fieldName);
    }
  }
  while ( VXIMapGetNextProperty( mapIterator, &key, &value ) == VXIvalue_RESULT_SUCCESS );

  // Print the boundary terminator
  result += CRLF "--" SB_BOUNDARY "--" CRLF;

  VXIMapIteratorDestroy(&mapIterator);

  return result;
}


void SBinetURL::appendQueryArgsToMultipart(SBinetNString& result, const VXIValue *value, SBinetNString& fieldName)
{
  switch (VXIValueGetType(value))
  {
   case VALUE_CONTENT:
     {
       // audio
       const VXIchar* type;
       const VXIchar* transferEnc;
       const VXIbyte* data;
       VXIulong size;
       char sizeInt[10];

       appendKeyToMultipart(result, fieldName.c_str());

       VXIContentValue((const VXIContent *) value, &type, &data, &size);
       result += "Content-Type: ";
       result += type;
       result += CRLF;

       sprintf(sizeInt,"%lu",size);
       result += "Content-Length: ";
       result += sizeInt;
       result += CRLF;

       transferEnc = VXIContentGetTransferEncoding((const VXIContent *) value);
       if( transferEnc != NULL )
       {
           result += "Content-Transfer-Encoding: ";
           result += transferEnc;
           result += CRLF;
       }

       // One blank line before actual data.
       result += CRLF;

       result.append((const char*) data, size);
     }
     break;
   case VALUE_MAP:
     appendQueryArgsMapToMultipart(result, (const VXIMap *) value, fieldName);
     break;
   case VALUE_VECTOR:
     appendQueryArgsVectorToMultipart(result, (const VXIVector *)value, fieldName);
     break;
   default:
     appendKeyToMultipart(result, fieldName.c_str(), false);
     appendValueToMultipart(result, valueToNString(value));
     break;
  }
}

//
// Infer a MIME content type from a URL (by the extension)
//
VXIString *SBinetURL::getContentTypeFromUrl() const
{
  //
  // Determine the extension. This has to work for local files and also HTTP
  //
  const VXIchar *url = _strPath.c_str();

  if (url == NULL || !*url)
  {
    return NULL;
  }

  const VXIchar *urlEnd = url;

  // Search for the end of the URL.
  while (*urlEnd != L'?' && *urlEnd != L'#' && *urlEnd) urlEnd++;

  // Now look for the last '.'
  VXIchar ext[1024];
  const VXIchar *urlDot = urlEnd - 1;

  while (urlDot >= url && *urlDot != L'.') urlDot--;

  if (urlDot >= url)
  {
    ::wcsncpy(ext, urlDot, urlEnd - urlDot);
    ext[urlEnd - urlDot] = L'\0';

    if (*ext)
    {
      const VXIchar* typeFromMap = SBinetChannel::mapExtension(ext);
      if (typeFromMap != NULL)
      {
	if (*typeFromMap)
	  return VXIStringCreate(typeFromMap);
	else
	  return NULL;
      }

      // GN: Skip this step.  The default type will be returned instead.
      //     This is what happens on other OSes.  The problem is that if
      //     the file extension is .xml, FindMimeFromData() will return
      //     a MIME type of text/xml and this is not one of the supported
      //     types in SRGS.  Allow the user of INET to determine the type
      //     instead by returning the default type.
#if 0
      /*
       * Could not find in map, use Win32 if available
       */

#ifdef _WIN32
      // Try Windows, this supports a number of hardcoded MIME content
      // types as well as extension mapping rules. To define a new
      // extension mapping rule, create a new Win32 registry key called
      // HKEY_CLASSES_ROOT\.<ext> where <ext> is the extension name. Then
      // create a value under that called "Content Type" where the data is
      // the content type string, such as "audio/aiff". Browse that
      // location in the registry for numerous examples (not all have a
      // content type mapping though).
      //
      // TBD sniff the actual data buffer too as this permits, pass the
      // proposed MIME type too (as returned by the web server, if any).
      VXIchar *mimeBuf;
      if (FindMimeFromData (NULL, url, NULL, 0, NULL, 0, &mimeBuf, 0) !=
	  NOERROR)
	mimeBuf = NULL;

      /* FindMimeFromData( ) leaks, we add unknown extensions here to
	 make it so we only leak once per extension instead of
	 repeatedly */
      if (mimeBuf && *mimeBuf) {
	SBinetChannel::addExtensionRule(ext, mimeBuf);
	return VXIStringCreate(mimeBuf);
      } else {
	SBinetChannel::addExtensionRule(ext, L"");
      }
#endif
#endif
    }
  }

  /*
   * Couldn't figure out MIME type: return default type.
   */
  return NULL;
}
