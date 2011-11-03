
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
 * Basic VXI Types
 *
 * The following VXI types are used in the VXI interfaces and
 * implementation to improve portability. The basic VXI types are:
 *
 *   VXIchar        Locality dependent char type
 *   VXIbool        Boolean with values TRUE or FALSE
 *   VXIbyte        Single byte
 *   VXIint         Native platform int
 *   VXIunsigned    Native platform unsigned int
 *   VXIint32       32-bit int
 *   VXIlong        Native platform long
 *   VXIulong       Native platform unsigned long
 *   VXIflt32       32-bit IEEE float
 *   VXIflt64       64-bit IEEE float
 *   VXIptr         Untyped pointer
 *
 ************************************************************************
 */

#ifndef _VXITYPES_H
#define _VXITYPES_H

#ifdef __cplusplus
#include <cwchar>         /* C++ wchar_t support */
#else
#include <wchar.h>        /* C wchar_t support */
#endif
#ifndef WIN32
#include <pthread.h>      /* For pthread_t */
#endif

#include "VXIheaderPrefix.h"

 /**
  * \defgroup VXItypes Type definitions
  *
  * VXI prefixed primitive types to ensure portability across a wide
  * variety of operating systems. These types are used throughout all
  * the VXI interfaces as well as within implementations of those
  * interfaces.  
  */

/*@{*/

/**
 * i386-* bindings
 */

typedef unsigned int   VXIbool;
typedef unsigned char  VXIbyte;
typedef wchar_t        VXIchar;
typedef int            VXIint;
typedef unsigned int   VXIunsigned;
typedef int            VXIint32;
typedef long           VXIlong;
typedef unsigned long  VXIulong;
typedef float          VXIflt32;
typedef double         VXIflt64;
typedef void *         VXIptr;
#ifdef WIN32
typedef VXIlong        VXIthreadID;
#else
typedef pthread_t      VXIthreadID;
#endif

#ifdef __cplusplus
#include <string>
typedef std::basic_string<VXIchar> vxistring;
#endif

/**
 * Common MIME content types used for multiple interfaces
 */
#define VXI_MIME_SRGS              L"application/srgs+xml"
#define VXI_MIME_SSML              L"application/synthesis+ssml"
#define VXI_MIME_VXML              L"application/voicexml+xml"

#define VXI_MIME_TEXT              L"text/plain"
#define VXI_MIME_UNICODE_TEXT      L"text/plain;charset=wchar_t"
#define VXI_MIME_UTF16_TEXT        L"text/plain;charset=UTF-16"
#define VXI_MIME_XML               L"text/xml"

#define VXI_MIME_ALAW              L"audio/x-alaw-basic"
#define VXI_MIME_LINEAR            L"audio/L8;rate=8000"
#define VXI_MIME_LINEAR_16         L"audio/L16;rate=8000"
#define VXI_MIME_LINEAR_16_16KHZ   L"audio/L16;rate=16000"
#define VXI_MIME_ULAW              L"audio/basic"
#define VXI_MIME_VOX               L"audio/x-dialogic-vox"
#define VXI_MIME_WAV               L"audio/x-wav"

/**
 * Current VXI interface version
 */
#define VXI_CURRENT_VERSION     0x00030004
#define VXI_MAJOR_VERSION(x)    (((x) >> 16) & 0xFFFF)
#define VXI_MINOR_VERSION(x)    ((x) & 0xFFFF)
#define VXI_CURRENT_VERSION_STR L"3.4"

/**
 * True and false for VXIbool values
 */
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/*@}*/

#include "VXIheaderSuffix.h"

#endif /* end of include guard */
