
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

#ifndef _SWICHAR_H_
#define _SWICHAR_H_

#ifdef __cplusplus
#include <cwchar>
#else
#include <wchar.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup Character Character and String utilities
 */

/*@{*/

typedef enum {
  SWIchar_CONVERSION_LOSS = -6,  /**< the conversion resulted in character loss             */
  SWIchar_BUFFER_OVERFLOW = -5,  /**< the buffer is too small                               */
  SWIchar_OUT_OF_MEMORY = -4,    /**< malloc returns null                                   */
  SWIchar_INVALID_INPUT = -3,    /**< null, empty string, unicode when should be ascii only */
  SWIchar_FAIL = -2,             /**< unable to convert                                     */
  SWIchar_ERROR = -1,            /**< general error                                         */
  SWIchar_SUCCESS = 0            /**< success                                               */
} SWIcharResult;

/// Tests whether a character is a whitespace character.
#define SWIchar_isspace(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' || \
                        (x) == '\r' || (x) == '\013' || (x) == '\014')
/// Tests whether a wide character is a whitespace character.
#define SWIchar_iswspace(x) ((x) == L' ' || (x) == L'\t' || (x) == L'\n' || \
                        (x) == L'\r' || (x) == L'\013' || (x) == L'\014')

/// Tests whether a character is a digit.
#define SWIchar_isdigit(x) ((x) >= '0' && (x) <= '9')
/// Tests whether a wide character is a digit.
#define SWIchar_iswdigit(x) ((x) >= L'0' && (x) <= L'9')

/// Tests whether a character is alphanumeric.
#define SWIchar_isalpha(x) (((x) >= 'a' && (x) <= 'z') || \
    ((x) >= 'A' && (x) <= 'Z'))
/// Tests whether a wide character is alphanumeric.
#define SWIchar_iswalpha(x) (((x) >= L'a' && (x) <= L'z') || \
    ((x) >= L'A' && (x) <= L'Z'))

#define SWIchar_MAXSTRLEN    64

const wchar_t *SWIcharReturnCodeToWcs(SWIcharResult rc);


/*@}*/

#ifdef __cplusplus
}
#endif

#endif


