
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

/* (1) Platform specific macro which handles symbol exports & imports */

#ifndef SWIUTIL_HEADER_PREFIX_H
#define SWIUTIL_HEADER_PREFIX_H

#ifdef _WIN32
#ifdef __cplusplus
#define SWIUTIL_EXPORT_DECL extern "C" __declspec(dllexport)
#define SWIUTIL_IMPORT_DECL extern "C" __declspec(dllimport)
#define SWIUTIL_EXPORT_CPP_DECL extern __declspec(dllexport)
#define SWIUTIL_IMPORT_CPP_DECL extern __declspec(dllimport)
#define SWIUTIL_EXPORT_CPP_CLASS_DECL __declspec(dllexport)
#define SWIUTIL_IMPORT_CPP_CLASS_DECL __declspec(dllimport)
#else
#define SWIUTIL_EXPORT_DECL __declspec(dllexport)
#define SWIUTIL_IMPORT_DECL __declspec(dllimport)
#endif
#else
#ifdef __cplusplus
#define SWIUTIL_EXPORT_DECL extern "C"
#define SWIUTIL_IMPORT_DECL extern "C"
#define SWIUTIL_EXPORT_CPP_DECL extern
#define SWIUTIL_IMPORT_CPP_DECL extern
#define SWIUTIL_EXPORT_CPP_CLASS_DECL
#define SWIUTIL_IMPORT_CPP_CLASS_DECL
#else
#define SWIUTIL_EXPORT_DECL extern
#define SWIUTIL_IMPORT_DECL extern
#endif
#endif

#if !defined(SWIUTIL_EXPORT_DECL) || !defined(SWIUTIL_IMPORT_DECL)
#error Symbol import/export pair not defined.
#endif

#ifdef SWIUTIL_NO_DLL

#define SWIUTIL_API_CLASS
#define SWIUTIL_API_CPP
#define SWIUTIL_API

#else  /* ! SWIUTIL_NO_DLL */

#ifdef SWIUTIL_EXPORTS

#ifdef __cplusplus
#define SWIUTIL_API_CLASS SWIUTIL_EXPORT_CPP_CLASS_DECL
#define SWIUTIL_API_CPP SWIUTIL_EXPORT_CPP_DECL
#endif
#define SWIUTIL_API SWIUTIL_EXPORT_DECL

#else  /* ! SWIUTIL_EXPORTS */

#ifdef __cplusplus
#define SWIUTIL_API_CLASS SWIUTIL_IMPORT_CPP_CLASS_DECL
#define SWIUTIL_API_CPP SWIUTIL_IMPORT_CPP_DECL
#endif
#define SWIUTIL_API SWIUTIL_IMPORT_DECL

#endif /* SWIUTIL_EXPORTS */

#endif /* SWIUTIL_NO_DLL */

#endif /* include guard */
