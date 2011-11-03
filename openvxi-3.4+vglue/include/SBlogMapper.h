
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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#ifndef SBLOG_MAPPER_HPP
#define SBLOG_MAPPER_HPP

#include "VXIheaderPrefix.h"
#ifdef SBLOGMAPPER_DLL
#ifdef SBLOGMAPPER_EXPORTS
#define SBLOGMAPPER_API SYMBOL_EXPORT_DECL
#else
#define SBLOGMAPPER_API SYMBOL_IMPORT_DECL
#endif
#else
#ifdef __cplusplus
#define SBLOGMAPPER_API extern "C"
#else
#define SBLOGMAPPER_API extern
#endif
#endif

#include "VXIlog.h"             // For VXIlogResult
#include "VXIvalue.h"           // For VXIVector

#ifdef __cplusplus
///
class SBlogErrorMapper;
#else
typedef struct SBlogErrorMapper { void * dummy; } SBlogErrorMapper;
#endif

/**
 * \addtogroup VXIlog
 */

/*@{*/

/**
 * Create a new XML error mapper
 *
 * @param errorMapFiles   [IN] VXIVector of local OpenSpeech Browser PIK
 *                             XML error mapping files
 * @param mapper          [OUT] Handle to the error mapper
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult 
SBlogErrorMapperCreate(const VXIVector    *errorMapFiles,
		       SBlogErrorMapper  **mapper);

/**
 * Destroy an XML error mapper
 *
 * @param mapper          [IN/OUT] Handle to the error mapper, set
 *                                 to NULL on success
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult
SBlogErrorMapperDestroy(SBlogErrorMapper **mapper);

/**
 * Map an error ID to text and a severity
 *
 * @param mapper      [IN] Handle to the error mapper
 * @param errorID     [IN] Error ID to map as passed to VXIlog::Error( )
 * @param moduleName  [IN] Module name reporting the error as passed to
 *                         VXIlog::Error( )
 * @param errorText   [OUT] Error text as defined in the error mapping
 *                          file. Owned by the error text mapper, must
 *                          not be modified or freed.
 * @param severity    [OUT] Severity identifier as defined in the error
 *                          mapping file. Owned by the error text mapper,
 *                          must not be modified or freed. Typically one
 *                          of the following:
 *                            0 -> UNKNOWN (no mapping found)
 *                            1 -> CRITICAL
 *                            2 -> SEVERE
 *                            3 -> WARNING
 *
 * @result VXIlog_RESULT_SUCCESS on success 
 */
SBLOGMAPPER_API VXIlogResult
SBlogErrorMapperGetErrorInfo(SBlogErrorMapper  *mapper,
			     VXIunsigned        errorID,
			     const VXIchar     *moduleName,
			     const VXIchar    **errorText,
			     VXIint            *severityLevel); 

/*@}*/

#include "VXIheaderSuffix.h"

#endif /* include guard */
