
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

#ifndef _VXICLIENT_UTILS_H
#define _VXICLIENT_UTILS_H

#include "VXIheaderPrefix.h"
#ifdef VXIUTIL_EXPORTS
#define VXIUTIL_API SYMBOL_EXPORT_DECL
#else
#define VXIUTIL_API SYMBOL_IMPORT_DECL
#endif

#include <VXIvalue.h>          /* For the VXIlog interface */
#include "VXIplatform.h"

#include <stdarg.h>               /* For va_list */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUFSIZE (4096 + 1024) // typical maxlen is 4096, want room over that
// Convert wide to narrow characters
#define w2c(w) (((w) & 0xff00)?'\277':((unsigned char) ((w) & 0x00ff)))

/* Avoid locale dependent ctype.h macros */
#define ISWSPACE(c) \
  (((c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r')) ? 1 : 0)

#define ISWALPHA(c) \
  ((((c >= L'a') && (c <= L'z')) || ((c >= L'A') && (c <= L'Z'))) ? 1 : 0)

#define ISWDIGIT(c) \
  (((c >= L'0') && (c <= L'9')) ? 1 : 0)

/* Avoid locale dependent mbstowcs( ) and wcstombs( ) */
VXIplatformResult
char2wchar(VXIchar *wstr, const char *str, int len);

VXIplatformResult 
wchar2char(char *str, const VXIchar *wstr, int len);

/* Convinence functions to retrieve VXIMap value */
int GetVXIBool
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIbool       *paramValue
);

int GetVXIInt
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIint32       *paramValue
);

int GetVXILong
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIlong       *paramValue
);

int GetVXIULong
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIulong      *paramValue
);

int GetVXIFloat
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIflt32      *paramValue
);

int GetVXIDouble
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  VXIflt64      *paramValue
);

int GetVXIString
(
  const VXIMap  *configArgs,
  const VXIchar *paramName,
  const VXIchar **paramValue
);

int GetVXIVector
(
  const VXIMap     *configArgs,
  const VXIchar    *paramName,
  const VXIVector **paramValue
);

int GetVXIMap
(
  const VXIMap     *configArgs,
  const VXIchar    *paramName,
  const VXIMap **paramValue
);

/* OS-independent vswprintf replacement */
int VXIvswprintf
(
  wchar_t* wcs, 
  size_t maxlen,
  const wchar_t* format, 
  va_list args
);


/**
 * Dynamic library loading
 */

#ifdef WIN32
#include <windows.h>
typedef HINSTANCE LibraryPtr;
#else
typedef void *    LibraryPtr;
#endif

typedef struct DynLibrary {
  LibraryPtr libPtr;
} DynLibrary;

VXIplatformResult
LoadDynLibrary
(
  const char      *libName, 
  DynLibrary      **libHandle
);

VXIplatformResult
LoadDynSymbol
(
  DynLibrary  *libHandle, 
  const char  *symName,
  void        **symPtr
);

VXIplatformResult
CloseDynLibrary
(
  DynLibrary      **libHandle
);


#ifdef __cplusplus
}
#endif

#endif /* include guard */
