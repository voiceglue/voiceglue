
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


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>                       // For CLK_TCK/CLOCKS_PER_SEC
#include <sys/times.h>                  // For times( )

#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif
#endif

#include <sys/timeb.h>                  // for ftime( )/_ftime( )
#include <sys/stat.h>                   // for stat( )

#include "SBlogOSUtils.h"
#include "VXIclientUtils.h"

#define BUFSIZE (4096 + 1024) // typical maxlen is 4096, want room over that

// Convert wide to narrow characters
#define w2c(w) (((w) & 0xff00)?'\277':((unsigned char) ((w) & 0x00ff)))

/*****************************
// SBlogGetTime
*****************************/
extern "C" int SBlogGetTime(time_t *timestamp, 
          VXIunsigned *timestampMsec)
{
#ifdef WIN32
  struct _timeb tbuf;
  _ftime(&tbuf);
  *timestamp = tbuf.time;
  *timestampMsec = (VXIunsigned) tbuf.millitm;
#else
  struct timeb tbuf;
  ftime(&tbuf);
  *timestamp = tbuf.time;
  *timestampMsec = (VXIunsigned) tbuf.millitm;
#endif

  return 0;
}


/*****************************
// SBlogGetTimeStampStr
*****************************/
extern "C" int SBlogGetTimeStampStr(time_t          timestamp,
            VXIunsigned     timestampMsec,
            char           *timestampStr)
{
#ifdef WIN32
  char *timeStr = ctime(&timestamp);
#else
  char timeStr_r[64] = "";
  char *timeStr = ctime_r(&timestamp, timeStr_r);
#endif
  if (timeStr) {
    // Strip the weekday name from the front, year from the end,
    // append hundredths of a second (the thousandths position is
    // inaccurate, remains constant across entire runs of the process)
    strncpy(timestampStr, &timeStr[4], 15);
    sprintf(&timestampStr[15], ".%02u", timestampMsec / 10);
  } else {
    timestampStr[0] = '\0';
    return -1;
  }

  return 0;
}

/*****************************
// SBlogGetFileStats
*****************************/
extern "C" int SBlogGetFileStats(const char *path, 
         SBlogFileStats *fileStats)
{
  int rc;
  #ifdef WIN32
  struct _stat stats;
  #else
  struct stat stats;
  #endif

  if ((! path) || (! fileStats))
    return -1;
  
  #ifdef WIN32
  rc = _stat(path, &stats);
  #else
  rc = stat(path, &stats);
  #endif
  
  if (rc != 0) {
    return -1;
  }
  
  fileStats->st_size  = stats.st_size;
  fileStats->st_mode  = stats.st_mode;
  fileStats->st_atim = stats.st_atime;
  fileStats->st_mtim = stats.st_mtime;
  fileStats->st_ctim = stats.st_ctime;
  
  return 0;
}

/*****************************
// SBlogGetCPUTimes
*****************************/
extern "C" int SBlogGetCPUTimes(long *userTime
                                /* ms spent in user mode */,
                                long *kernelTime
                                /* ms spent in kernel mode*/)
{
#ifdef WIN32
  FILETIME dummy;
  FILETIME k, u;
  LARGE_INTEGER lk, lu;

  if ((! userTime) || (! kernelTime))
    return -1;

  if (GetThreadTimes(GetCurrentThread(), &dummy, &dummy, &k, &u) == FALSE)
    return -1;

  lk.LowPart  = k.dwLowDateTime;
  lk.HighPart = k.dwHighDateTime;
  *kernelTime = (long) (lk.QuadPart / 10000);

  lu.LowPart  = u.dwLowDateTime;
  lu.HighPart = u.dwHighDateTime;
  *userTime   = (long) (lu.QuadPart / 10000);


#else
  struct tms timeBuf;

  if ((! userTime) || (! kernelTime))
    return -1;

  times(&timeBuf);
  *userTime = (long)timeBuf.tms_utime * 1000 / CLK_TCK;
  *kernelTime = (long)timeBuf.tms_stime * 1000 / CLK_TCK;

#endif
  return 0;
}


/*****************************
// format_wcs2str (internal-only)
*****************************/
static int format_wcs2str(const wchar_t *wformat, char *nformat)
{
  size_t len, i;
  bool replacement = false;
  
  len = wcslen(wformat);
  for (i = 0; i <= len; i++) {
    nformat[i] = w2c(wformat[i]);
    if (nformat[i] == '%') {
      if (replacement)
        replacement = false; // double %%
      else
        replacement = true;
    } else if ((replacement == true) && 
              (((nformat[i] >= 'a') && (nformat[i] <= 'z')) ||
              ((nformat[i] >= 'A') && (nformat[i] <= 'Z')))) {
      switch (nformat[i]) {
        case 's':
          // wide insert for wide format -> wide insert for narrow format
          nformat[i] = 'S';
          break;
        case 'S':
          // narrow insert for wide format -> narrow insert for narrow format
          nformat[i] = 's';
          break;
        default:
          break;
      }
      replacement = false;
    }
  }
  nformat[i] = '\0';
  
  return len;
}


/*****************************
// SBlogVswprintf
*****************************/
#ifndef WIN32
#if ! (defined(__GNUC__) && (__GNUC__ <= 2))
static wchar_t* ConvertFormatForWidePrintf(const wchar_t* format)
{
  int formatLen = wcslen(format);

  // The maximum length of the new format string is 1 and 2/3 that
  //    of the original.
  wchar_t* newFormat = new wchar_t [2 * formatLen];
  int nfIndex = 0;

  for (int fIndex = 0; fIndex < formatLen; ++fIndex)
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

extern "C" int SBlogVswprintf(wchar_t* wcs, size_t maxlen,
            const wchar_t* format, va_list args)
{
  int rc;

  if (maxlen < 1)
    return -1;
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

  /* Use a temporary buffer for output to protect against relatively
     small overruns */
  char *buf, tmpbuf[BUFSIZE];
  if (BUFSIZE < maxlen + 1024)
    buf = new char[maxlen + 1024];
  else
    buf = tmpbuf;
  if (!buf)
    return -1;

  /* convert format to narrow, a bit generous compared to the ANSI/ISO
     C specifications for the printf( ) family format strings but we're
     not trying to do full validation here anyway */
  char *fmt, tmpfmt[BUFSIZE];
  size_t fmtlen = wcslen(format);
  if (BUFSIZE < fmtlen + 1)
    fmt = new char[fmtlen + 1];
  else
    fmt = tmpfmt;
  if (!fmt)
    return -1;

  format_wcs2str(format, fmt);

  /* generate the final string based on the narrow format and arguments */ 
  rc = vsprintf(buf, fmt, args);

  /* copy back to wide characters */
  size_t finallen = strlen(buf);
  if (finallen >= maxlen)
    finallen = maxlen - 1;
  for (size_t i = 0; i < finallen; i++)
    wcs[i] = buf[i];
  wcs[finallen] = L'\0';

  /* clean up */
  if (buf != tmpbuf)
    delete [] buf;
  if (fmt != tmpfmt)
    delete [] fmt;

#else /* ! defined(__GNUC__) || (__GNUC__ > 2) */

  wchar_t* newFormat = ConvertFormatForWidePrintf(format);
  rc = vswprintf(wcs, maxlen, newFormat, args);
  delete [] newFormat;

  if ((size_t) rc >= maxlen - 1) /* overflow */
    wcs[maxlen - 1] = L'\0';

#endif /* defined(__GNUC__) && (__GNUC__ <= 2) */
#endif /* WIN32 */

  return rc;
}

/*****************************
// SBlogMkDir
*****************************/
int SBlogMkDir(const char *path)
{
#ifdef WIN32
#ifdef UNICODE
  wchar_t wpath[1024];
  char2wchar(wpath, path, strlen(path));
  return (CreateDirectory (wpath, NULL) ? 1 : 0);  
#else
  return (CreateDirectory (path, NULL) ? 1 : 0);
#endif  
#else
  return (mkdir (path, (mode_t)0755) == 0 ? 1 : 0);
#endif
}

/*****************************
// SBlogIsDir
*****************************/
int SBlogIsDir(const SBlogFileStats *statInfo)
{
#ifdef WIN32

#ifdef _S_ISDIR
  return (_S_ISDIR(statInfo->st_mode) ? 1 : 0);
#else
  return ((statInfo->st_mode & _S_IFDIR) ? 1 : 0);
#endif

#else   // ! WIN32

#ifdef S_ISDIR
  return (S_ISDIR(statInfo->st_mode) ? 1 : 0);
#else
  return ((statInfo->st_mode & S_IFDIR) ? 1 : 0);
#endif

#endif  // WIN32
}

/*****************************
// SBlogWchar2Latin1
*****************************/
extern "C" VXIbool SBlogWchar2Latin1(const wchar_t * input, char * output,
                                     VXIunsigned maxlen)
{
  const wchar_t * i;
  char * j;
  unsigned int idx = 0;
  
  if( output == NULL ) 
    return FALSE;

  if(input == NULL)
  {
    *output = '\0';
  }
  else
  {  
    for (i = input, j = output; *i && idx < maxlen; ++i, ++j, ++idx) 
    {
      char t = w2c(*i);
      *j = t;
    }
    *j = '\0';
  }

  return TRUE;
}


/*****************************
// SBlogWchar2UTF8
*****************************/
extern "C" VXIbool SBlogWchar2UTF8(const wchar_t * input, char * output,
           VXIunsigned maxOutputBytes,
           VXIunsigned * outputBytes)
{
  //  firstByteMark
  //      A list of values to mask onto the first byte of an encoded sequence,
  //      indexed by the number of bytes used to create the sequence.
  static const char firstByteMark[7] =
    {  char(0x00), char(0x00), char(0xC0), char(0xE0),
       char(0xF0), char(0xF8), char(0xFC) };

  //  Get pointers to our start and end points of the input buffer.
  const wchar_t* srcPtr = input;
  const wchar_t* srcEnd = srcPtr + wcslen(input);
  *outputBytes = 0;
  
  while (srcPtr < srcEnd) {
    wchar_t curVal = *srcPtr++;

    // Watchout for surrogates, if found truncate
    if ((curVal >= 0xD800) && (curVal <= 0xDBFF)) {
      break;
    }

    // Figure out how many bytes we need
    unsigned int encodedBytes;
    if (curVal < 0x80)                encodedBytes = 1;
    else if (curVal < 0x800)          encodedBytes = 2;
    else if (curVal < 0x10000)        encodedBytes = 3;
    else if (curVal < 0x200000)       encodedBytes = 4;
    else if (curVal < 0x4000000)      encodedBytes = 5;
    else if (curVal <= 0x7FFFFFFF)    encodedBytes = 6;
    else {
      // THIS SHOULD NOT HAPPEN!
      output[*outputBytes] = '\0';
      return FALSE;
    }

    //  If we don't have enough room in the buffer, truncate
    if ( *outputBytes + encodedBytes >= maxOutputBytes ) {
      break;
    }

    //  And spit out the bytes. We spit them out in reverse order
    //  here, so bump up the output pointer and work down as we go.
    char buffer[7] = { 0, 0, 0, 0, 0, 0, 0 };
    
    char * outPtr = buffer + encodedBytes;
    switch(encodedBytes) {
    case 6 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 5 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 4 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 3 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 2 : *--outPtr = char((curVal | 0x80) & 0xBF);
      curVal >>= 6;
    case 1 : *--outPtr = char(curVal | firstByteMark[encodedBytes]);
    }
    
    for (int i = 0; buffer[i] != 0; i++) {
      output[*outputBytes] = buffer[i];
      (*outputBytes)++;
    }
  }

  //  NULL terminate
  output[*outputBytes] = '\0';

  return TRUE;
}


/*****************************
// Log errors to the console, only used for errors that occur prior
// to initializing the log subsystem
*****************************/
VXIlogResult 
SBlogLogErrorToConsole(const VXIchar *moduleName, VXIunsigned errorID,
           const VXIchar *errorIDText)
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  const VXIchar *severity;
  if (errorID < 200)
    severity = L"CRITICAL";
  else if (errorID < 300)
    severity = L"SEVERE";
  else
    severity = L"WARNING";

#ifdef WIN32
  fwprintf(stderr, L"%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
     errorIDText);
#else
  fprintf(stderr, "%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
    errorIDText);
#endif

  return rc;
}


VXIlogResult 
SBlogVLogErrorToConsole(const VXIchar *moduleName, VXIunsigned errorID,
      const VXIchar *errorIDText, const VXIchar *format,
      va_list arguments)
{
  int argCount;
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  const VXIchar *in;
  VXIchar *out, tempFormat[256];

  /* Get the severity */
  const VXIchar *severity;
  if (errorID < 200)
    severity = L"CRITICAL";
  else if (errorID < 300)
    severity = L"SEVERE";
  else
    severity = L"WARNING";

  /* Properly delimit the format arguments for human consumption */
  argCount = 0;
  if (format) {
    for (in = format, out = tempFormat; *in != L'\0'; in++, out++) {
      if (*in == L'%') {
  argCount++;
  *out = (argCount % 2 == 0 ? L'=' : L'|');
  out++;
      }
      *out = *in;
    }
    *out = L'\n';
    out++;
    *out = L'\0';
    
#ifdef WIN32
    fwprintf(stderr, L"%ls: %ls|%u|%ls", severity, moduleName, errorID,
       errorIDText);
    vfwprintf(stderr, tempFormat, arguments);
#else
    /* SBlogVswprintf( ) handles the format conversion if necessary
       from Microsoft Visual C++ run-time library/GNU gcc 2.x C
       library notation to GNU gcc 3.x C library (and most other UNIX
       C library) notation: %s -> %ls, %S -> %s. Unfortunately there
       is no single format string representation that can be used
       universally. */
    VXIchar tempBuf[4096];
    SBlogVswprintf(tempBuf, 4096, tempFormat, arguments);
    fprintf(stderr, "%ls: %ls|%u|%ls%ls\n", severity, moduleName, errorID,
      errorIDText, tempBuf);
#endif
  } else {
#ifdef WIN32
    fwprintf(stderr, L"%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
       errorIDText);
#else
    fprintf(stderr, "%ls: %ls|%u|%ls\n", severity, moduleName, errorID,
      errorIDText);
#endif
  }
  
  return rc;
}
