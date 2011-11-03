
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
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>

#include "SWIprintf.h"
#include "misc_string.h"

static const char BAD_FORMAT_TEXT[] = " [bad format!]";
static const wchar_t BAD_FORMAT_TEXT_W[] = L" [bad format!]";
static const size_t BAD_FORMAT_TEXT_LEN = 14;

// Convert wide to narrow characters
#define w2c(w) (((w) & 0xff00)?'\277':((unsigned char) ((w) & 0x00ff)))

#if !defined(_win32_)
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
      while((SWIchar_iswdigit(format[fIndex]) || (format[fIndex] == L'+') ||
            (format[fIndex] == L'-') || (format[fIndex] == L'.') ||
            (format[fIndex] == L'*') || (format[fIndex] == L'#')) &&
            (fIndex < formatLen - 1)) {
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

static char* ConvertFormatForNarrowPrintf(const char* format)
{
  int formatLen = strlen(format);

  // The maximum length of the new format string is 1 and 2/3 that
  //    of the original.
  char* newFormat = new char [2 * formatLen];
  int nfIndex = 0;

  for (int fIndex = 0; fIndex < formatLen; ++fIndex)
  {
    // We found %s in format.
    if ((format[fIndex] == '%') && (fIndex != (formatLen - 1)))
    {
      newFormat[nfIndex++] = '%';
      fIndex++;
      while((SWIchar_isdigit(format[fIndex]) || (format[fIndex] == L'+') ||
            (format[fIndex] == L'-') || (format[fIndex] == L'.') ||
            (format[fIndex] == L'*') || (format[fIndex] == L'#')) &&
            (fIndex < formatLen - 1)) {
        newFormat[nfIndex++] = format[fIndex++];
      }

      if (format[fIndex] == 'S')
      {
        newFormat[nfIndex++] = 'l';
        newFormat[nfIndex++] = 's';
      }
      else if (format[fIndex] == 'C')
      {
        newFormat[nfIndex++] = 'l';
        newFormat[nfIndex++] = 'c';
      }
      else
      {
        newFormat[nfIndex++] = format[fIndex];
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

static char* ConvertWideFormatForNarrowPrintf(const wchar_t* format)
{
  int formatLen = wcslen(format);

  // The maximum length of the new format string is 1 and 2/3 that
  //    of the original.
  char* newFormat = new char [2 * formatLen];
  int nfIndex = 0;

  for (int fIndex = 0; fIndex < formatLen; ++fIndex)
  {
    // We found %s in format.
    if ((format[fIndex] == L'%') && (fIndex != (formatLen - 1)))
    {
      newFormat[nfIndex++] = '%';
      fIndex++;
      while((SWIchar_iswdigit(format[fIndex]) || (format[fIndex] == L'+') ||
            (format[fIndex] == L'-') || (format[fIndex] == L'.') ||
            (format[fIndex] == L'*') || (format[fIndex] == L'#')) &&
            (fIndex < formatLen - 1)) {
        newFormat[nfIndex++] = w2c(format[fIndex++]);
      }

      switch(format[fIndex]) {
        case L's': {
          newFormat[nfIndex++] = 'l';
          newFormat[nfIndex++] = 's';
          break;
        }
        case L'S': {
          newFormat[nfIndex++] = 's';
          break;
        }
        case L'c': {
          newFormat[nfIndex++] = 'l';
          newFormat[nfIndex++] = 'c';
          break;
        }
        case L'C': {
          newFormat[nfIndex++] = 'c';
          break;
        }
        default: {
          newFormat[nfIndex++] = w2c(format[fIndex]);
          break;
        }
      }
    }
    else
    {
      newFormat[nfIndex++] = w2c(format[fIndex]);
    }
  }

  newFormat[nfIndex] = 0;

  return newFormat;
}
#endif

/*
 *  Common functions for all OSes
 */

SWIPRINTF_API int SWIfprintf(FILE* file, const char* format, ...)
{
  va_list ap;
  int rc;

  va_start(ap, format);

#if 0
  try {
#endif
#if !defined(_win32_)
    char* newFormat = ConvertFormatForNarrowPrintf(format);
    rc = vfprintf(file, newFormat, ap);
    delete []newFormat;
#else
    rc = vfprintf(file, format, ap);
#endif
#if 0
  } catch(...) {
    rc = fputs(format, file);
  }
#endif
  va_end(ap);

  return rc;
}


SWIPRINTF_API int SWIfwprintf(FILE* file, const wchar_t* format, ...)
{
  va_list ap;
  int rc;

  va_start(ap, format);

#if !defined(_win32_)
  char* newFormat = ConvertWideFormatForNarrowPrintf(format);
  rc = vfprintf(file, newFormat, ap);
  delete [] newFormat;
#else
  rc = vfwprintf(file, format, ap);
#endif

  va_end(ap);

  return rc;
}


SWIPRINTF_API int SWIsprintf(char* str, size_t maxlen, const char* format, ...)
{
  va_list ap;
  int rc;

  va_start(ap, format);
  rc = SWIvsprintf(str, maxlen, format, ap);
  va_end(ap);

  return rc;
}


  // if maxlen == 0, wcs is untouched, returns -1
  // if there is overflow, returns -1 and maxlen-1 # of chars are written
SWIPRINTF_API int SWIswprintf (wchar_t* wcs, size_t maxlen,
			       const wchar_t* format, ...)
{
  va_list ap;
  int rc;
  wchar_t strbuf[200]; // try to avoid malloc
  wchar_t *tstr;

  if (maxlen < 1)
    return -1;

  if ( maxlen >= 200 ) {
    tstr = (wchar_t *)malloc( (maxlen+1)*sizeof(wchar_t) );
    if (tstr == NULL) {
      //FIXME-- can't log, how can we notify user?
      return -1;
    }
  } else {
    tstr = strbuf;
  }

  va_start(ap, format);
  rc = SWIvswprintf(tstr, maxlen, format, ap);
  va_end(ap);

  wcscpy( wcs,tstr );

  if ( tstr != strbuf ) free(tstr);

  return rc;
}


/*
 *  OS specific functions below, UNIX variants start first
 */

#ifndef _win32_
static int wcs2str(const wchar_t *wcs, char *str)
{
  int len, i;

  len = wcslen(wcs);
  for (i = 0; i < len; i++)
    str[i] = w2c(wcs[i]);
  str[i] = '\0';

  return len;
}

static int str2wcs(const char *str, wchar_t *wcs)
{
  int len, i;

  len = strlen(str);
  for (i = 0; i < len; i++)
    wcs[i] = (wchar_t)str[i];
  wcs[i] = L'\0';

  return len;
}

/* convert str to wcs */
static int buf_str2wcs(const char *str, wchar_t *wcs, int len)
{
  int i;

  for (i = 0; i < len; i++)
    wcs[i] = (wchar_t)str[i];

  return len;
}

static int buf_wcs2str(const wchar_t *wcs, char *str, int len)
{
  int i;

  for (i = 0; i < len; i++)
    str[i] = w2c(wcs[i]);

  return len;
}


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
    }
    else if ((replacement == true) && (SWIchar_isalpha(nformat[i]))) {
      switch (nformat[i]) {
      case 'c':
	// wide insert for wide format -> wide insert for narrow format
	nformat[i] = 'C';
	break;
      case 's':
	// wide insert for wide format -> wide insert for narrow format
	nformat[i] = 'S';
	break;
      case 'C':
	// narrow insert for wide format -> narrow insert for narrow format
	nformat[i] = 'c';
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


SWIPRINTF_API int SWIvsprintf(char* str, size_t maxlen,
			      const char* format, va_list args)
{
  char tmpbuf[BUFSIZE];
  char *buf;
  int rc;

  if (maxlen < 1)
    return -1;
  str[0] = '\0';

  /* allocate buffer, allow for overrun */
  buf = BUFMALLOC(tmpbuf, maxlen + 1024);
  if (!buf)
    return -1;

#if 0
  try {
#endif
    char* newFormat = ConvertFormatForNarrowPrintf(format);
    rc = vsnprintf(buf, maxlen, newFormat, args);
    delete []newFormat;
#if 0
  } catch(...) {
    if (strlen(format) < maxlen) {
      strcpy(buf, format);
      rc = strlen(buf);
      if (rc + BAD_FORMAT_TEXT_LEN < maxlen) {
	strcat(buf, BAD_FORMAT_TEXT);
	rc += BAD_FORMAT_TEXT_LEN;
      }
    }
  }
#endif

  /* copy buffer back */
  if (rc >= 0) {
    size_t len = strlen(buf);
    if (len < maxlen) {
      strcpy(str, buf);
    } else {
      strncpy(str, buf, maxlen - 1);
      str[maxlen - 1] = '\0';
    }
  }

  BUFFREE(tmpbuf, buf);

  return rc;
}


SWIPRINTF_API int SWIvswprintf(wchar_t* wcs, size_t maxlen,
			       const wchar_t* format, va_list args)
{
  int rc;

  if (maxlen < 1)
    return -1;
  wcs[0] = '\0';

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
  rc = vsnprintf(buf, maxlen, fmt, args);

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

  // vswprintf() returns a -1 on overflow but does not terminate
  //   the string with a NULL character.
  // Terminate the string with a NULL character in either of
  //   these cases.
  if ((size_t) rc >= maxlen - 1 || (size_t) rc == -1) /* overflow */
    wcs[maxlen - 1] = L'\0';

#endif /* defined(__GNUC__) && (__GNUC__ <= 2) */

  return rc;
}


SWIPRINTF_API wchar_t *SWIfgetws(wchar_t *ws, int n, FILE *stream)
{
  char tmpbuf[BUFSIZE];
  char *buf;
  char *rs;

  /* allocate local buffer */
  buf = BUFMALLOC(tmpbuf, n);
  if (!buf) {
    return NULL;
  }

  rs = fgets(buf, n, stream);
  if (rs == NULL) {
    BUFFREE(tmpbuf, buf);
    return NULL;
  }

  str2wcs(buf, ws);
  BUFFREE(tmpbuf, buf);

  return ws;
}

/* Only for linux: Solaris has a swscanf but doesn't have vsscanf!! */
#if defined(_linux_)
SWIPRINTF_API int SWIswscanf(const wchar_t *s, const wchar_t *format, ... )
{
  char tmpbuf[BUFSIZE];
  char *buf;
  char tmpfmt[BUFSIZE];
  char *fmt;
  va_list ap;
  int rc;

  buf = BUFMALLOC(tmpbuf, wcslen(s) + 1);
  if (!buf) {
    return 0;
  }

  fmt = BUFMALLOC(tmpfmt, wcslen(format) + 1);
  if (!fmt) {
    BUFFREE(tmpbuf, buf);
    return 0;
  }

  /* convert buffers */
  wcs2str(s, buf);
  format_wcs2str(format, fmt);

  va_start(ap, format);
  rc = vsscanf(buf, fmt, ap);
  va_end(ap);

  /* we do not need to convert anything back */
  BUFFREE(tmpbuf, buf);
  BUFFREE(tmpfmt, fmt);

  return rc;
}
#endif

#elif defined(_win32_)

SWIPRINTF_API int SWIvsprintf(char* str, size_t maxlen,
			      const char* format, va_list args)
{
  int rc;

  if (maxlen < 1)
    return -1;
  str[0] = '\0';

#if 0
  try {
#endif
    rc = _vsnprintf(str, maxlen, format, args);
    // vsnprintf() returns a -1 on overflow but does not terminate
    //   the string with a NULL character.
    // Terminate the string with a NULL character in either of
    //   these cases.
    if ((size_t) rc >= maxlen - 1 || (size_t) rc == -1) /* overflow */
      str[maxlen - 1] = '\0';
#if 0
  } catch(...) {
    if (strlen(format) < maxlen) {
      strcpy(str, format);
      rc = strlen(str);
      if (rc + BAD_FORMAT_TEXT_LEN < maxlen) {
	strcat(str, BAD_FORMAT_TEXT);
	rc += BAD_FORMAT_TEXT_LEN;
      }
    }
  }
#endif

  return rc;
}


SWIPRINTF_API int SWIvswprintf(wchar_t* wcs, size_t maxlen,
			       const wchar_t* format, va_list args)
{
  int rc;

  if (maxlen < 1)
    return -1;
  wcs[0] = '\0';

#if 0
  try {
#endif
    rc = _vsnwprintf(wcs, maxlen, format, args);

    // vsnwprintf() returns a -1 on overflow but does not terminate
    //   the string with a NULL character.
    // Terminate the string with a NULL character in either of
    //   these cases.
    if ((size_t) rc >= maxlen - 1 || (size_t) rc == -1) /* overflow */
      wcs[maxlen - 1] = L'\0';
#if 0
  } catch(...) {
    if (wcslen(format) < maxlen) {
      wcscpy(wcs, format);
      rc = wcslen(wcs);
      if (rc + BAD_FORMAT_TEXT_LEN < maxlen) {
	wcscat(wcs, BAD_FORMAT_TEXT_W);
	rc += BAD_FORMAT_TEXT_LEN;
      }
    }
  }
#endif

  return rc;
}



SWIPRINTF_API wchar_t *SWIfgetws(wchar_t *ws, int n, FILE *stream)
{
  return fgetws(ws, n, stream);
}


/* SWSCANF on Win32 is just a #define to the real version */



#else
#error
#endif
