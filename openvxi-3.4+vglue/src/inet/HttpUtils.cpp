
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

#include "SBinetString.hpp"
#include "HttpUtils.hpp"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ap_ctype.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#else
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/select.h>
#endif


static void escapeDataIntern(const VXIbyte *data,
                             VXIulong size,
                             SBinetHttpUtils::HttpUriEncoding mask,
                             SBinetNString &result)
{
  /*
   *	Bit 0		xalpha
   *	Bit 1		xpalpha		-- as xalpha but with plus.
   *	Bit 2 ...	path		-- as xpalpha but with /
   */
  static const VXIbyte isAcceptable[96] =
    /*0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
  {
    0x0,0x0,0x0,0x0,0x0,0xE,0x0,0x0,0x0,0x0,0xF,0xE,0x0,0xF,0xF,0xC, /* 2x  !"#$%&'()*+,-./   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x8,0x0,0x0,0x0,0x0,0x0, /* 3x 0123456789:;<=>?   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF, /* 4x @ABCDEFGHIJKLMNO   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0xF, /* 5X PQRSTUVWXYZ[\]^_   */
    0x0,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF, /* 6x `abcdefghijklmno   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0x0  /* 7X pqrstuvwxyz{\}~DEL */
  };

  static const char *hexChar = "0123456789ABCDEF";

  // Now perform the actual conversion
  const VXIbyte *p;
  VXIulong i;
  for (i = 0, p = data; i < size; i++, p++)
  {
    VXIbyte a = *p;
    if (a < 32 || a >= 128 || !(isAcceptable[a-32] & mask))
    {
      result += '%';
      result += hexChar[a >> 4];
      result += hexChar[a & 15];
    }
    else
      result += static_cast<char>(*p);
  }
}

void SBinetHttpUtils::escapeString(const char *str, HttpUriEncoding mask, SBinetNString &result)
{
  result.clear();

  if (str == NULL) return;

  escapeDataIntern((const VXIbyte *) str, strlen(str), mask, result);
}


void SBinetHttpUtils::escapeData(const void *data,
                                 const int size,
                                 HttpUriEncoding mask,
                                 SBinetNString& result)
{
  result.clear();
  if (data == NULL) return;
  escapeDataIntern((const VXIbyte *) data, size, mask, result);
}


void SBinetHttpUtils::utf8encode(const VXIchar *str, SBinetNString& result)
{
  result.clear();

  if (str == NULL) return;

  //  firstByteMark
  //      A list of values to mask onto the first byte of an encoded sequence,
  //      indexed by the number of bytes used to create the sequence.
  static const char firstByteMark[7] = {  char(0x00), char(0x00), char(0xC0), char(0xE0),
                                          char(0xF0), char(0xF8), char(0xFC) };

  for (const VXIchar *p = str; *p; p++)
  {
    VXIchar c = *p;

    if (c >= 0xD800 && c <= 0xDBFF)
      break;

    unsigned int encodedBytes;
    if (c < 0x80)
      encodedBytes = 1;
    else if (c < 0x800)
      encodedBytes = 2;
    else if (c < 0x10000)
      encodedBytes = 3;
    else if (c < 0x200000)
      encodedBytes = 4;
    else if (c < 0x4000000)
      encodedBytes = 5;
    else if (c <= 0x7FFFFFFF)
      encodedBytes = 6;
    else
    {
      result.clear();
      break;
    }

    //  And spit out the bytes. We spit them out in reverse order
    //  here, so bump up the output pointer and work down as we go.
    char buffer[7] = { 0, 0, 0, 0, 0, 0, 0 };

    char * outPtr = buffer + encodedBytes;
    switch (encodedBytes)
    {
     case 6 :
       *--outPtr = char((c | 0x80) & 0xBF);
      c >>= 6;
     case 5 :
       *--outPtr = char((c | 0x80) & 0xBF);
      c >>= 6;
     case 4 :
       *--outPtr = char((c | 0x80) & 0xBF);
      c >>= 6;
     case 3 :
       *--outPtr = char((c | 0x80) & 0xBF);
      c >>= 6;
     case 2 :
       *--outPtr = char((c | 0x80) & 0xBF);
      c >>= 6;
     case 1 :
       *--outPtr = char(c | firstByteMark[encodedBytes]);
    }

    for (int i = 0; buffer[i] != 0; i++)
      result += buffer[i];
  }
}
const char *SBinetHttpUtils::getToken(const char *str, SBinetNString& token)
{
  static const char *tspecials = "()<>@,;:\\\"/[]?={} \t";

  const char *beg = str;

  while (*beg && ap_isspace(*beg)) beg++;
  if (!*beg) return NULL;

  const char *end = beg;

  while (*end && !strchr(tspecials, *end) && !ap_iscntrl(*end)) end++;
  if (end == beg)
  {
    return NULL;
  }

  token.clear();
  token.append(beg, end - beg);
  return end;
}

const char *SBinetHttpUtils::getQuotedString(const char *str,
					     SBinetNString& value)
{
  // skip leading '"'
  ++str;
  value.clear();

  while (*str && *str != '"')
  {
    switch (*str)
    {
     case '\\':
       str++;
       if (*str) value += *str;
       break;
     default:
       value += *str;
       break;
    }
    str++;
  }

  if (*str == '"')
    return ++str;

  return NULL;
}

const char *SBinetHttpUtils::getValue(const char *str, SBinetNString& value)
{
  const char *beg = str;
  while (*beg && ap_isspace(*beg)) beg++;
  if (!*beg) return NULL;

  if (*beg == '"')
  {
    return getQuotedString(beg, value);
  }
  else
  {
    return getToken(beg, value);
  }
}

const char *SBinetHttpUtils::getCookieValue(const char *str,
					    SBinetNString& value)
{
  const char *beg = str;
  while (*beg && ap_isspace(*beg)) beg++;
  if (!*beg) return NULL;

  if (*beg == '"')
  {
    return getQuotedString(beg, value);
  }
  else
  {
    // The specification says that a value is either a token or a quoted
    // string.  However, a lot of broken servers just don't quote string
    // containing blanks.  We have to include everything up to the semicolon.
#ifdef NO_BROKEN_SERVER
    return getToken(beg, value);
#else
    while (*beg && ap_isspace(*beg)) beg++;
    if (!*beg) return NULL;
    const char *end = beg;
    while (*end && *end != ';')
    {
      // some really broken servers even put a comma in the value.  In this
      // case, we scan the string to find the first occurence of a semi-colon
      // or an equal sign.  If we find the semi-colon first, we assume that
      // the comma is part of the value and the value terminates at the
      // semi-colon location.  Otherwise, the value terminates with the comma.
      if (*end == ',')
      {
        const char *orig_end = end;
        while (*end && *end != ';' && *end != '=') end++;
        if (*end == '=') end = orig_end;
        break;
      }
      end++;
    }

    // removing trailing blanks.
    const char *lastChar = end - 1;
    while (lastChar >= beg && ap_isspace(*lastChar)) lastChar--;
    lastChar++;

    value.clear();
    value.append(beg, lastChar - beg);
    return end;
#endif
  }
}

const char *SBinetHttpUtils::expectChar(const char *start, const char *delim)
{
  while (*start && ap_isspace(*start)) start++;

  if (!*start || strchr(delim, *start))
    return start;

  return NULL;
}


// Sleep with a duration in microseconds
void SBinetHttpUtils::usleep(unsigned long microsecs)
{
#ifdef WIN32
  Sleep (microsecs / 1000);
#else
  fd_set fdset;
  struct timeval timeout;

  FD_ZERO(&fdset);
  timeout.tv_sec = microsecs / 1000000;
  timeout.tv_usec = microsecs % 1000000;
  select(0, &fdset, &fdset, &fdset, &timeout);
#endif
}


// Case insensitive compare
int SBinetHttpUtils::casecmp(const wchar_t *s1, const wchar_t *s2)
{
  register unsigned int u1, u2;

  for (;;) {
    u1 = (unsigned int) *s1++;
    u2 = (unsigned int) *s2++;
    if (toUpper(u1) != toUpper(u2)) {
      return toUpper(u1) - toUpper(u2);
    }
    if (u1 == '\0') {
      break;
    }
  }

  return 0;
}


int SBinetHttpUtils::casecmp(const wchar_t *s1, const wchar_t *s2,
			     register size_t n)
{
  register unsigned int u1, u2;

  for (; n != 0; --n) {
    u1 = (unsigned int) *s1++;
    u2 = (unsigned int) *s2++;
    if (toUpper(u1) != toUpper(u2)) {
      return toUpper(u1) - toUpper(u2);
    }
    if (u1 == '\0') {
      break;
    }
  }

  return 0;
}
