
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

#ifndef _SWISTRING_H_
#define _SWISTRING_H_

#include <SWIchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup Character
 */

/*@{*/

  /* li_string.cpp */

/// Convert a string to double 
extern double SWIatof( const char *str);
/// Convert a string to float
extern float SWIatofloat( const char *str);
/// Convert a double to a string
extern SWIcharResult SWIdtostr(double d, char *str, int len);
/// Convert a double to a wide string
extern SWIcharResult SWIdtowcs(double d, wchar_t *wstr, int len);


  /* SWIstring.c */

/// Convert a wide string to double 
double SWIwcstod( const wchar_t *wstr);
/// Convert a wide string to float 
float SWIwcstof( const wchar_t *wstr);
/// Convert a wide string to float
SWIcharResult SWIwtof(const wchar_t *wstr, float *fval);

/// Convert a narrow string to wide
SWIcharResult SWIstrtowcs(const char *str, wchar_t *wstr, int len);
/// Convert a wide string to narrow
SWIcharResult SWIwcstostr(const wchar_t *wstr, char *str, int len);
/// Convert a integer to a wide string
SWIcharResult SWIitowcs(int i, wchar_t *wstr, int len);
/// Convert a integer to a wide string
SWIcharResult SWIwcstoi(const wchar_t *wstr, int *pi);
/// Convert a wide string to an integer
int SWIwtoi(const wchar_t *wstr);
/// Compares a wide string to a narrow string
int SWIwcsstrcmp(const wchar_t *w, const char *str);

/**
 * Tests that the given wchar string contains only ASCII 
 * characters, which are any character with a value less 
 * than than or equal to 0x7F.
 *
 * @param wstr The wide string to test
 * @return non-zero if the string is all ascii
 */
int SWIisascii(const wchar_t *wstr);

/**
 * Tests that the given wchar string contains only LATIN-1 
 * characters, which are any character with a value less 
 * than than or equal to 0xFF.
 *
 * @param wstr The wide string to test
 * @return non-zero if the string is all Latin-1
 */
int SWIislatin1(const wchar_t *wstr);

/**
 * Tests that the given wchar string
 *    - does not contain high surrogates (D800 to DBFF)
 *    - does not contain non-characters (FFFE and FFFF)
 *    - the top 16-bit of 32-bit wchar are 0
 *
 * @param wstr The wide string to test
 * @return non-zero if the string is all Unicode.
 */
int SWIisvalid_unicode(const wchar_t *wstr);

/// Find the next token in a wide string.
wchar_t* SWIwcstok (wchar_t* wcs, const wchar_t* delim, wchar_t** ptr);

/// Find the next token in a string.
char *SWIstrtok(char *str, const char *delim, char **ptr);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
