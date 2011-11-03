
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

#ifndef SBLOG_OS_UTILS
#define SBLOG_OS_UTILS

#include <stdarg.h>
#include <time.h>
#include <wchar.h>
#include <limits.h>
#include <VXItypes.h>
#include <VXIlog.h>

/* Only support 8 bit unicode char */
#if (CHAR_BIT != 8)
#error REQUIRING 8 BIT REPRESENTATIONS FOR TYPE 'char'!
#endif
#if (CHAR_MIN == -127)
#error CANNOT HANDLE ONES COMPLEMENT NOTATION!
#endif

#ifdef __cplusplus
extern "C" {
#endif

int SBlogGetTime(time_t       *timestamp, 
                 VXIunsigned  *timestampMsec);

int SBlogGetTimeStampStr(time_t          timestamp,
                         VXIunsigned     timestampMsec,
                         char*           timestampStr);

typedef struct SBlogFileStats
{
  long int st_size;
  long int st_mode;
  time_t st_atim;
  time_t st_mtim;
  time_t st_ctim;

} SBlogFileStats;

int SBlogGetFileStats(const char      *path, 
                      SBlogFileStats  *fileStats);

int SBlogGetCPUTimes(long *userTime
                     /* ms spent in user mode */,
                     long *kernelTime
                     /* ms spent in kernel mode*/);

int SBlogVswprintf(wchar_t        *wcs, size_t     maxlen, 
                   const wchar_t  *format, va_list args);

/* platform independent directory manipulation */
int SBlogMkDir(const char *path);
int SBlogIsDir(const SBlogFileStats *statInfo);

/* platform independent wide char -> char conversion util */
VXIbool SBlogWchar2Latin1(const wchar_t * input, char * output,
        VXIunsigned maxOutputChars);
VXIbool SBlogWchar2UTF8(const wchar_t * input, char * output,
      VXIunsigned maxOutputBytes, VXIunsigned * outputBytes);

VXIlogResult SBlogLogErrorToConsole(const VXIchar *moduleName, 
            VXIunsigned errorID,
            const VXIchar *errorIDText);

VXIlogResult SBlogVLogErrorToConsole(const VXIchar *moduleName, 
             VXIunsigned errorID,
             const VXIchar *errorIDText, 
             const VXIchar *format,
             va_list arguments);
  
#ifdef __cplusplus
}

/* Avoid locale dependant ctype.h macros */
static inline bool SBlogIsSpace(const char c) {
  return (((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r')) ? 
    true : false);
}

static inline bool SBlogIsSpace(const wchar_t c) {
  return (((c == L' ') || (c == L'\t') || (c == L'\n') || (c == L'\r')) ?
    true : false);
}

static inline bool SBlogIsAlpha(const wchar_t c) {
  return ((((c >= L'a') && (c <= L'z')) || ((c >= L'A') && (c <= L'Z'))) ?
    true : false);
}

#endif /* __cplusplus */

#endif /* include guard */
