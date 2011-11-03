
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

#ifndef SWIUTIL_INTERNAL_H
#define SWIUTIL_INTERNAL_H

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifdef WIN32
#define wcsncasecmp(s1,s2,n) _wcsnicmp(s1,s2,n)
#define wcscasecmp(s1,s2) _wcsicmp(s1,s2)
#define strcasecmp(a,b) _stricmp(a,b)
#endif

#ifdef __cplusplus

namespace std { };
using namespace std;

#ifdef _MSC_VER           /* Microsoft Visual C++ */

/* Suppress/disable warnings triggered by STL headers */
#pragma warning(4 : 4786) /* identifier was truncated to '255' 
			     characters in the debug information */

#endif /* _MSC_VER */

#if defined(__GNUC__) && (! defined(_GLIBCPP_USE_WCHAR_T))

#include <string>

namespace std
{
  typedef basic_string<wchar_t> wstring;
}

#endif

#endif /* __cplusplus */

#endif /* include guard */
