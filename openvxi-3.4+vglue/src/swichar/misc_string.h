/* misc_string, miscellaneous internal string utility macros */

/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004 -2005 by Vocalocity, Inc. All Rights Reserved.
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

#ifndef __MISC_STRING_H
#define __MISC_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#define BUFSIZE (4096 + 1024) // typical maxlen is 4096, want room over that

#define BUFMALLOC(buf, len) \
  ((sizeof(buf) >= (len)) ? (buf) : (char *) malloc(len))

#define BUFMALLOC2(buf, currlen, len) \
  ((currlen >= len) ? (buf) : (char *) malloc(len))

#define BUFFREE(buf, val) \
  if ((val) != (buf)) free(val)


#ifdef __cplusplus
}
#endif

#endif
