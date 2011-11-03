
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

/************************************************************************
 *
 *
 * Settings which should start all public headers
 *
 ************************************************************************
 */

/* (1) Platform specific macro which handles symbol exports & imports */

#ifndef _VXIHEADERPREFIX_ONE_TIME_
#ifdef WIN32
  #ifdef __cplusplus
    #define SYMBOL_EXPORT_DECL extern "C" __declspec(dllexport)
    #define SYMBOL_IMPORT_DECL extern "C" __declspec(dllimport)
    #define SYMBOL_EXPORT_CPP_DECL extern __declspec(dllexport)
    #define SYMBOL_IMPORT_CPP_DECL extern __declspec(dllimport)
    #define SYMBOL_EXPORT_CPP_CLASS_DECL __declspec(dllexport)
    #define SYMBOL_IMPORT_CPP_CLASS_DECL __declspec(dllimport)
  #else
    #define SYMBOL_EXPORT_DECL __declspec(dllexport)
    #define SYMBOL_IMPORT_DECL __declspec(dllimport)
  #endif
#else
  #ifdef __cplusplus
    #define SYMBOL_EXPORT_DECL extern "C"
    #define SYMBOL_IMPORT_DECL extern "C"
    #define SYMBOL_EXPORT_CPP_DECL extern
    #define SYMBOL_IMPORT_CPP_DECL extern
    #define SYMBOL_EXPORT_CPP_CLASS_DECL
    #define SYMBOL_IMPORT_CPP_CLASS_DECL
  #else
    #define SYMBOL_EXPORT_DECL extern
    #define SYMBOL_IMPORT_DECL extern
  #endif
#endif

#if !defined(SYMBOL_EXPORT_DECL) || !defined(SYMBOL_IMPORT_DECL)
#error Symbol import/export pair not defined.
#endif
#endif /* end of (1) */


/* Define structure packing conventions */

#ifdef WIN32
#if defined(_MSC_VER)            /* Microsoft Visual C++ */
  #pragma pack(push, 8)
#elif defined(__BORLANDC__)      /* Borland C++ */
  #pragma option -a8
#elif defined(__WATCOMC__)       /* Watcom C++ */
  #pragma pack(push, 8)
#else                            /* Anything else */
  #error Review the settings for your compiler.
#endif

/* Do other (one time) compiler specific work */

#ifndef _VXIHEADERPREFIX_ONE_TIME_
  #if defined(_MSC_VER)
    #include "VXIcompilerMsvc.h"
  #endif
#endif

#endif /*win32*/

/* Define compiler guard for one-time instructions */

#if !defined(_VXIHEADERPREFIX_ONE_TIME_)
#define _VXIHEADERPREFIX_ONE_TIME_
#endif
