
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

#ifndef _SBINET_LOG_H
#define _SBINET_LOG_H

#include "VXIlog.h"          /* Logging interface */
#ifdef __cplusplus
#include "SBinetLogger.hpp"  /* Logging base class */
#endif

/* Module defines */
#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif

#ifdef OPENVXI
#define MODULE_SBINET                  MODULE_PREFIX L"OSBinet"
#define SBINET_IMPLEMENTATION_NAME     COMPANY_DOMAIN L".OSBinet"
#else
#define MODULE_SBINET                  MODULE_PREFIX L"SBinet"
#define SBINET_IMPLEMENTATION_NAME     COMPANY_DOMAIN L".SBinet"
#endif

#define MODULE_SBINET_API_TAGID             0
#define MODULE_SBINET_CHANNEL_TAGID         1
#define MODULE_SBINET_STREAM_TAGID          2
#define MODULE_SBINET_COOKIE_TAGID          3
#define MODULE_SBINET_VALIDATOR_TAGID       4
#define MODULE_SBINET_CACHE_TAGID           5
#define MODULE_SBINET_TIMING_TAGID          6
#define MODULE_SBINET_PROXY_TAGID           7
#define MODULE_SBINET_CONNECTION_TAGID      8
#define MODULE_SBINET_DUMP_HTTP_MSGS        10

/* Only for SBinetLibwww, preserved for OpenVXI distributions */
#define MODULE_SBINET_HTALERT_TAGID         100
#define MODULE_SBINET_HTPRINT_TAGID         101
#define MODULE_SBINET_HTTRACE_TAGID         102
#define MODULE_SBINET_HTTRACEDATA_TAGID     103
#define MODULE_SBINET_HTTP_HEADERS_TAGID    104

#endif  /* _SBINET_LOG_H */
