
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

#ifndef _SWI_PRINTF_H__
#define _SWI_PRINTF_H__

#include <stdio.h>
#include <stdarg.h>
#include <SWIchar.h>

/* This is required for OSB reference source distributions of this
 * file, where we have to get these functions exported without a def
 * file. */
#ifdef SWIPRINTF_DLL_EXPORT
#ifdef __cplusplus
#define SWIPRINTF_API extern "C" __declspec(dllexport)
#else
#define SWIPRINTF_API __declspec(dllexport)
#endif
#else
#ifdef __cplusplus
#define SWIPRINTF_API extern "C"
#else
#define SWIPRINTF_API extern
#endif
#endif

/**
 * \addtogroup Character
 */

/*@{*/

///
SWIPRINTF_API
int SWIfprintf (FILE* file, const char* format, ...);

///
SWIPRINTF_API
int SWIfwprintf (FILE* file, const wchar_t* format, ...);

///
SWIPRINTF_API
int SWIsprintf (char* str, size_t maxlen, const char* format, ...);

///
SWIPRINTF_API
int SWIswprintf (wchar_t* wcs, size_t maxlen, const wchar_t* format, ...);

///
SWIPRINTF_API
int SWIvsprintf (char* str, size_t maxlen, const char* format, va_list args);

///
SWIPRINTF_API
int SWIvswprintf (wchar_t* wcs, size_t maxlen, const wchar_t* format,
		  va_list args);

///
SWIPRINTF_API
wchar_t* SWIfgetws (wchar_t* ws, int n, FILE* stream);

#ifdef _linux_
SWIPRINTF_API
int SWIswscanf(const wchar_t* buffer, const wchar_t* format, ... );
#elif defined(_win32_)
#define SWIswscanf swscanf
#elif defined(_solaris_)
#define SWIswscanf swscanf
#else
/* Assume we have swscanf() and see what happens */
#define SWIswscanf swscanf
#endif

/*@}*/

#endif  /* _SWI_PRINTF_H__ */
