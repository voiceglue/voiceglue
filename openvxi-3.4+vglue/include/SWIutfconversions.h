
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

#ifndef _SWIUTFCONVERSIONS_H_
#define _SWIUTFCONVERSIONS_H_

#include <stdio.h>
#include <SWIchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup Character
 */

/*@{*/

/**
 * Wide to UTF-8 conversion.
 * @param src The source wide string
 * @param dst The destination buffer the will hold the UTF-8 result.
 * @param maxdstlen The maximum number of character that dst should contain.
 * @return -1 for buffer overflow, otherwise the length of the dst string.
 */
SWIcharResult SWIwcstoutf8(const wchar_t *src, unsigned char *dst, int maxdstlen);

/**
 * UTF-8 to wide conversion.
 * @param src The source UTF-8 string
 * @param dst The destination buffer the will hold the wide result.
 * @param maxdstlen The maximum number of character that dst should contain.
 * @return -1 for buffer overflow, otherwise the length of the dst string.
 */
SWIcharResult SWIutf8towcs( const unsigned char *src, wchar_t *dst, int maxdstlen );

/**
 * Returns the length of a UTF-8 string after conversion from wide.
 */
int SWIwcstoutf8len(const wchar_t* src);

/**
 * Returns the length of a wide string after conversion from UTF-8.
 */
int SWIutf8towcslen(const unsigned char* src);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
