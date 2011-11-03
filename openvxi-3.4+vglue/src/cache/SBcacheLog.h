
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

#ifndef _SBCACHE_LOG_H
#define _SBCACHE_LOG_H

#include "VXIlog.h"          /* Logging interface */
#ifdef __cplusplus
#include "SBinetLogger.hpp"  /* Logging base class */
#endif

/* Module defines */
#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif

#ifdef OPENVXI
#define MODULE_SBCACHE                 MODULE_PREFIX L"OSBcache"
#define SBCACHE_IMPLEMENTATION_NAME    COMPANY_DOMAIN L".OSBcache"
#else
#define MODULE_SBCACHE                 MODULE_PREFIX L"SBcache"
#define SBCACHE_IMPLEMENTATION_NAME    COMPANY_DOMAIN L".SBcache"
#endif

#define SBCACHE_API_TAGID         0     /* API log */
#define SBCACHE_MGR_TAGID         1     /* Cache manager log */
#define SBCACHE_ENTRY_TAGID       2     /* Cache entry log */
#define SBCACHE_STREAM_TAGID      3     /* Cache stream log */
#define SBCACHE_ET_MUTEX_TAGID    4     /* Entry table mutex log */
#define SBCACHE_E_MUTEX_TAGID     5     /* Entry mutex log */
#define SBCACHE_CLEANUP_TAGID     6     /* Cache cleanup */

#endif  /* _SBCACHE_LOG_H */
