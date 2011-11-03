
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

/* -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8
 */

#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#define VXIUTIL_EXPORTS
#include "VXIclientUtils.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif
#define MODULE_SBCLIENT MODULE_PREFIX L"VXIclient"


/* -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8
 */

/* Converts char-based string to wchar-based string
 * - len is size of wstr
 * - returns VXIplatform_RESULT_BUFFER_TOO_SMALL if len(str)>=len
 */
VXIplatformResult char2wchar(wchar_t *wstr, const char *str, int len)
{
  int l = strlen(str);
  int i;
  if (l > len)
    return VXIplatform_RESULT_BUFFER_TOO_SMALL;
    
  memset(wstr, 0, len);
  for (i = 0; i < l + 1; ++i)
    wstr[i] = (wchar_t)str[i];  /* the high bit is not a sign bit */
  return VXIplatform_RESULT_SUCCESS;
}


/* Converts wchar-based string to char-based string.
 *   and each loss-affected char is \277 (upside-down question mark)
 * - len is size of str
 * - returns VXIplatform_RESULT_BUFFER_TOO_SMALL if len(wstr)>=len
*/
VXIplatformResult wchar2char(char *str, const wchar_t *wstr, int len)
{
  int l = wcslen(wstr);
  int i;
  if (l > len)
    return VXIplatform_RESULT_BUFFER_TOO_SMALL;

  memset(str, 0, len);
  for (i = 0; i < l + 1; i++)
    str[i] = w2c(wstr[i]);
  return VXIplatform_RESULT_SUCCESS;
}

#ifndef WIN32
#if ! (defined(__GNUC__) && (__GNUC__ <= 2))
static wchar_t* ConvertFormatForWidePrintf(const wchar_t* format)
{
  int formatLen = wcslen(format);
  // The maximum length of the new format string is 1 and 2/3 that
  //    of the original.
  wchar_t* newFormat = (wchar_t*)malloc(sizeof(wchar_t) * (2 * formatLen + 1));
  int nfIndex = 0, fIndex = 0;
  for (fIndex = 0; fIndex < formatLen; ++fIndex)
  {
    // We found %s in format.
    if ((format[fIndex] == L'%') && (fIndex != (formatLen - 1)))
    {
      newFormat[nfIndex++] = L'%';
      fIndex++;
      while ((fIndex < formatLen - 1) &&
       ((format[fIndex] >= L'0') && (format[fIndex] <= L'9')) ||
       (format[fIndex] == L'+') || (format[fIndex] == L'-') ||
       (format[fIndex] == L'.') || (format[fIndex] == L'*') ||
       (format[fIndex] == L'#')) {
        newFormat[nfIndex++] = format[fIndex++];
      }

      switch(format[fIndex]) {
        case L's': {
          newFormat[nfIndex++] = L'l';
          newFormat[nfIndex++] = L's';
          break;
        }
        case L'S': {
          newFormat[nfIndex++] = L's';
          break;
        }
        case L'c': {
          newFormat[nfIndex++] = L'l';
          newFormat[nfIndex++] = L'c';
          break;
        }
        case L'C': {
          newFormat[nfIndex++] = L'c';
          break;
        }
        default: {
          newFormat[nfIndex++] = format[fIndex];
          break;
        }
      }
    }
    else
    {
      newFormat[nfIndex++] = format[fIndex];
    }
  }
  newFormat[nfIndex] = 0;
  return newFormat;
}
#endif /* ! defined(__GNUC__) && (__GNUC__ <= 2) */
#endif /* ! WIN32 */

/* OS-independent vswprintf replacement */
int VXIvswprintf
(
  wchar_t* wcs, 
  size_t maxlen,
  const wchar_t* format, 
  va_list args
)
{
  int rc;
  if (maxlen < 1) return -1;
  *wcs = 0;

#ifdef WIN32
  /* Straight-forward Win32 implementation */
  rc = _vsnwprintf(wcs, maxlen, format, args);
  if ((size_t) rc >= maxlen - 1) /* overflow */
    wcs[maxlen - 1] = L'\0';
#else /* ! WIN32 */
#if defined(__GNUC__) && (__GNUC__ <= 2)
  /* Some versions of the GNU C library do not provide the required
     vswprintf( ) function, so we emulate it by converting the format
     string to narrow characters, do a narrow sprintf( ), then convert
     back */
  char *buf = (char*)malloc(sizeof(char) * (maxlen + 512)); // want extra room
  if (!buf)
    return -1;

  /* convert format to narrow, a bit generous compared to the ANSI/ISO
     C specifications for the printf( ) family format strings but we're
     not trying to do full validation here anyway */
  size_t fmtlen = wcslen(format);
  char *fmt = (char*)malloc(sizeof(char) * (fmtlen + 1));
  if( !fmt )
    return -1;

  wchar2char(fmt, format, fmtlen);

  /* generate the final string based on the narrow format and arguments */ 
  rc = vsprintf(buf, fmt, args);
  /* copy back to wide characters */
  size_t finallen = strlen(buf);
  if (finallen >= maxlen) finallen = maxlen - 1;
  char2wchar(wcs, buf, finallen);
  /* clean up */
  if( buf ) free(buf);
  if( fmt) free(fmt);
#else /* ! defined(__GNUC__) || (__GNUC__ > 2) */

  wchar_t* newFormat = ConvertFormatForWidePrintf(format);
  rc = vswprintf(wcs, maxlen, newFormat, args);
  if( newFormat ) free(newFormat);

  if ((size_t) rc >= maxlen - 1) /* overflow */
    wcs[maxlen - 1] = L'\0';

#endif /* defined(__GNUC__) && (__GNUC__ <= 2) */
#endif /* WIN32 */
  return rc;
}


/* Convenience functions to retrieve VXI value */
int GetVXIBool
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIbool       *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_INTEGER) {
      *paramValue = (VXIbool)VXIIntegerValue((const VXIInteger*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIInt
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIint32       *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_INTEGER) {
      *paramValue = VXIIntegerValue((const VXIInteger*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXILong
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIlong       *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_LONG) {
      *paramValue = VXILongValue((const VXILong*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIULong
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIulong      *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_ULONG) {
      *paramValue = VXIULongValue((const VXIULong*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIFloat
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIflt32      *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_FLOAT) {
      *paramValue = VXIFloatValue((const VXIFloat*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIDouble
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIflt64      *paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_DOUBLE) {
      *paramValue = VXIDoubleValue((const VXIDouble*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIString
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  const VXIchar **paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_STRING) {
      *paramValue = VXIStringCStr((const VXIString*)v);    
      return 1;
    }
  }
  return 0; 
}

int GetVXIVector
(
  const VXIMap     *configArgs,
  const VXIchar    *paramName,
  const VXIVector **paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_VECTOR) {
      *paramValue = (const VXIVector*)v;
      return 1;
    }  
  }
  return 0;  
}

int GetVXIMap
(
  const VXIMap     *configArgs,
  const VXIchar    *paramName,
  const VXIMap **paramValue
)
{
  if( configArgs && paramName && paramValue ) {
    const VXIValue *v = VXIMapGetProperty(configArgs, paramName);
    if( v && VXIValueGetType(v) == VALUE_MAP) {
      *paramValue = (const VXIMap*)v;
      return 1;
    }  
  }
  return 0;    
}

/**
 * Dynamic library loading
 */

VXIplatformResult
LoadDynLibrary
(
  const char  *libName, 
  DynLibrary  **libHandle
)
{
  LibraryPtr libPtr;
  DynLibrary *libImpl = NULL;
  if ((! libName) || (! *libName) || (! libHandle))
    return VXIplatform_RESULT_FATAL_ERROR;

#ifdef WIN32

#ifdef UNICODE
  {
    int len = strlen(libName);
    wchar_t *wlibName = (wchar_t*)malloc(sizeof(wchar_t*)*(len+1));
    if( !wlibName ) return VXIplatform_OUT_OF_MEMORY;
    char2wchar(wlibName, libName, len+1);
    libPtr = LoadLibrary(wlibName);
    free(wlibName);
  }
#else
  libPtr = LoadLibrary(libName);
#endif

#else
  libPtr = dlopen(libName, RTLD_LAZY);
#endif

  if (libPtr) {
    libImpl = (DynLibrary *) 
      malloc(sizeof(DynLibrary));
    if (libImpl)
      libImpl->libPtr = libPtr;
  } 
  *libHandle = (DynLibrary *) libImpl;
  return (*libHandle ? VXIplatform_RESULT_SUCCESS :
	                     VXIplatform_RESULT_FATAL_ERROR);
}


VXIplatformResult
LoadDynSymbol
(
  DynLibrary      *libHandle, 
  const char      *symName,
  void            **symPtr
)
{
  if ((! libHandle) || (! symName) || (! *symName) || (! symPtr))
    return VXIplatform_RESULT_FATAL_ERROR;

#ifdef WIN32

#ifdef UNICODE
  {
    int len = strlen(symName);
    wchar_t *wsymName = (wchar_t*)malloc(sizeof(wchar_t*)*(len+1));
    if( !wsymName ) return VXIplatform_OUT_OF_MEMORY;
    char2wchar(wsymName, libName, len+1);    
    (*symPtr) = (void *) GetProcAddress(libHandle->libPtr, wsymName);
    free(wsymName);
  }
#else
  (*symPtr) = (void *) GetProcAddress(libHandle->libPtr, symName);
#endif

#else
  (*symPtr) = (void *) dlsym(libHandle->libPtr, symName);
#endif

  return (*symPtr ? VXIplatform_RESULT_SUCCESS :
	                  VXIplatform_RESULT_FATAL_ERROR);
}


VXIplatformResult
CloseDynLibrary
(
  DynLibrary      **libHandle
)
{
  if( libHandle && *libHandle ) {
#ifdef WIN32
    FreeLibrary((*libHandle)->libPtr);
#else
    dlclose((*libHandle)->libPtr);
#endif
    free(*libHandle );
    *libHandle = NULL;  
  }
  return VXIplatform_RESULT_SUCCESS;
}

