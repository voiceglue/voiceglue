
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

#ifndef _SBINET_H
#define _SBINET_H

#include "VXIinet.h"                   /* For VXIinet base interface */
#include "VXIlog.h"                    /* For VXIlog interface */
#include "VXIcache.h"                  /* For VXIcache interface */

#include "VXIheaderPrefix.h"
#ifdef SBINET_EXPORTS
#define SBINET_API SYMBOL_EXPORT_DECL
#else
#define SBINET_API SYMBOL_IMPORT_DECL
#endif
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup SBIinet VXIinetInterface Implementation
 *
 * SBinet interface, and implementation of VXIinetInterface
 * for Internet functionality, including HTTP requests, local
 * file access, URL caching, memory buffer caching, and cookie
 * access. <p>
 *
 * The interface is a synchronous interface based on the ANSI/ISO C
 * standard file I/O interface, the only exception is that pre-fetches are
 * asynchronous. The client of the interface may use this in an
 * asynchronous manner by using non-blocking I/O operations, creating
 * threads, or by invoking this from a separate server process. <p>
 *
 * This implementation currently does NOT support INET_MODE_WRITE for
 * http:// access (HTTP PUT), and only supports http://, file://, and
 * OS dependant paths.<p>
 *
 * There is one Internet interface per thread/line.
 */

/*@{*/

/**
 * Default value for the User Agent Name parameter of SBinetInit()
 */
#define SBINET_USER_AGENT_NAME_DEFAULT \
              L"OpenVXITestApp/" VXI_CURRENT_VERSION_STR

/**
 * @name SBInet Properties
 * Properties used in the VXIMap configParams arguments to SBinetInitEx. 
 */

/*@{*/

/** 
 * Rules for mapping file extensions to MIME content types, used for that
 * purpose when accessing local files and file:// URLs. The associated value
 * is a VXIMap whose keys must be an extension (period followed by the
 * extension such as ".txt") with the value being the MIME content type for
 * that extension.
 */
#define SBINET_EXTENSION_RULES       L"com.vocalocity.inet.extension.rules"

/**
 * Rules for determining the proxy to use for given domains.  The associated
 * value is a VXIVector whose entries are VXIString representing a key-value
 * pair delimited by a vertical bar ('|') the key (left of the vertical bar)
 * represents partial URLs to which a proxy applies, and the value is of the
 * form of proxy:port representing the proxy/port combination to be used for
 * the corresponding domain.  The partial URLs are of the form domain/path
 * where the left of the slash ('/') represents a domain (and must start
 * with a dot '.' or be empty) and the part after the slash represents paths
 * on this domain.  If the slash is omitted, then all paths in this domain
 * apply.  Each entry are verified for a match in the order in which they
 * appear in the VXIVector.  Also, if nothing is specified in the proxy
 * field (after the vertical bar), it means that no proxy is used
 *
 * An entry matches a URL if its domain is a suffix of the URL's domain and
 * its path is a prefix of the URL's path.


   For Example:  Assuming the following vector
   1)   ".vocalocity.com/specialPath  | proxy1:port1"
   2)   ".vocalocity.com              |"
   3)   ".com                         | proxy1:port1"
   4)   "                             | proxy2:port2"


   The following URL would use the following proxy.

   www.vocalocity.com                            -> no proxy (rule 2)
   www.vocalocity.com/specialPath/index.html     -> proxy1:port1 (rule1)
   www.foo.bar.com                               -> proxy1:port1 (rule3)
   www.mit.edu                                   -> proxy2:port2 (rule4)

   Also note that if order of rule-1 and rule-2 were reversed, than the
   ".vocalocity.com/specialPath" rule would never be triggered as it is more
   specific than the ".vocalocity.com" rule.
 */
#define SBINET_PROXY_RULES           L"com.vocalocity.inet.proxy.rules"

/**
 * Default timeout for fetching a page when not explicitely specified
 * through the INET_TIMEOUT_DOWNLOAD property in the properties VXIMap
 * argument of the VXIinterface::Open() function.  This value must be a
 * VXIInteger and the units are expressed in milliseconds.  If not
 * specified, the default timeout used is 60000 (1 minute).
 */
#define SBINET_PAGE_LOADING_TIMEOUT  L"com.vocalocity.inet.pageLoadingTimeout"

/** Default value for SBINET_PAGE_LOADING_TIMEOUT */
#define SBINET_PAGE_LOADING_TIMEOUT_DEFAULT  60000

/** 
 * Properties used to estimate the cache lifetime of URIs that only have the
 * Last-Modified header defined.
 *
 * SBINET_FRESHNESS_FRACTION must be a VXIFloat between 0 and 1.<p>
 *
 * SBINET_FRESHNESS_LIFETIME must be a VXIInteger whose units are in
 * milliseconds.<p>
 *
 * The freshness lifetime of an HTTP document having only Last-Modified
 * information is computed as the minimum between the amount of time since
 * the last modification multiplied by SBINET_FRESHNESS_FRACTION and
 * SBINET_FRESHNESS_LIFETIME.<p>
 *
 * freshness = MIN((now - lastMod) * SBINET_FRESHNESS_FRACTION,
 *                 SBINET_FRESHNESS_LIFETIME)
 *
 * Default values are 0.1 for SBINET_FRESHNESS_FRACTION and 86400 (24 hours)
 * for SBINET_FRESHNESS_LIFETIME.
 */
#define SBINET_FRESHNESS_FRACTION    L"com.vocalocity.inet.freshnessFraction"
#define SBINET_FRESHNESS_LIFETIME    L"com.vocalocity.inet.freshnessLifetime"

 /** Default value for SBINET_FRESHNESS_FRACTION */
#define SBINET_FRESHNESS_FRACTION_DEFAULT 0.1

/** Default value for SBINET_FRESHNESS_LIFETIME */
#define SBINET_FRESHNESS_LIFETIME_DEFAULT 86400

/** 
 * If returned paged from web server do not contain any caching information
 * nor any Last-Modified header, this is the amount of time (number of
 * seconds) that the page will be considered fresh before a new HTTP request
 * is performed for this page.  This value must be a VXIInteger and defaults
 * to 0 (immediate expiration) if not specified.
 */
#define SBINET_MAX_LIFETIME          L"com.vocalocity.inet.maxLifetime"

/** Default value for SBINET_MAX_LIFETIME */
#define SBINET_MAX_LIFETIME_DEFAULT 0

/**
 * Number of milliseconds that a POST-request should wait for a response.
 * This value must be a VXIInteger.  Default value is 5000 (5 seconds).
 */
#define SBINET_POST_CONTINUE_TIMEOUT L"com.vocalocity.inet.postContinueTimeout"

/** Default value for SBINET_POST_CONTINUE_TIMEOUT */
#define SBINET_POST_CONTINUE_TIMEOUT_DEFAULT  5000

/**
 * Property specifiying tsent in all HTTP messages.  If specified, it must
 * be a VXIString of the form <app>/<version> with no spaces, such as
 * "OpenVXI/1.0". When using the OpenVXI application name or a derivative,
 * use use VXI_CURRENT_VERSION_STR for the version.  Default value is
 * specified by SBINET_USER_AGENT_NAME_DEFAULT.
 */
#define SBINET_USER_AGENT_NAME L"com.vocalocity.inet.userAgentName"

/**
 * Properties allowing to disable persistent connections.  If this property
 * is specified and is a VXIInteger with a value of 0, then a new connection
 * is established, by default, for each HTTP request.  If this property is
 * not specified or is not a VXIInteger with a value of 0, then persistent
 * connections are enabled.  This property is overridden by the
 * INET_CLOSE_CONNECTION and the INET_NEW_CONNECTION properties that can
 * be passed to the Open() method of the VXIinet interface.
 */
#define SBINET_PERSISTENT_CONNECTIONS L"com.vocalocity.inet.usePersistentConnections"

/** 
 * Default MIME type returned when it was not possible to determine the
 * MIME type of a file URI from its extension or when an HTTP server
 * does not return MIME type information.
 */
#define SBINET_DEFAULT_MIME_TYPE L"com.vocalocity.inet.defaultMimeType"

/*@}*/

/**
 * Global platform initialization of SBinet
 *
 * @param log              VXI Logging interface used for error/diagnostic
 *                         logging, only used for the duration of this
 *                         function call
 * @param reserved1        Reserved for future use
 * @param reserved2        Reserved for future use
 * @param reserved3        Reserved for future use
 * @param reserved4        Reserved for future use
 * @param proxyServer      Name of the proxy server to use for HTTP access,
 *                         pass a server name or IP address, or NULL to
 *                         do direct HTTP access.
 * @param proxyPort        Port number for accessing the proxy server.
 * @param userAgentName    HTTP user agent name sent in all HTTP messages.
 *                         Must be of the form <app>/<version> with no
 *                         spaces, such as "OpenVXI/1.0". When using the
 *                         OpenVXI application name or a derrivative, use
 *                         use VXI_CURRENT_VERSION_STR for the version.
 * @param extensionRules   Rules for mapping file extensions to MIME content
 *                         types, used for that purpose when accessing
 *                         local files and file:// URLs. Each key in the
 *                         map must be an extension (period followed by the
 *                         extension such as ".txt") with the value being
 *                         the MIME content type for that extension.  Copied
 *                         internally so the pointer that is passed in still
 *                         belongs to the caller.
 * @param reserved         Reserved VXIVector, pass NULL
 *
 * @result VXIinet_RESULT_SUCCESS on success
 */
SBINET_API VXIinetResult SBinetInit(VXIlogInterface  *log,
                                    const VXIunsigned diagLogBase,
                                    const VXIchar    *reserved1,
                                    const VXIint      reserved2,
                                    const VXIint      reserved3,
                                    const VXIint      reserved4,
                                    const VXIchar    *proxyServer,
                                    const VXIulong    proxyPort,
                                    const VXIchar    *userAgentName,
                                    const VXIMap     *extensionRules,
                                    const VXIVector  *reserved);

/**
 * Global platform initialization (extended version) of SBinet.
 *
 * @param log              VXI Logging interface used for error/diagnostic
 *                         logging, only used for the duration of this
 *                         function call
 * @param diagLogBase      VXI Diagnostic Logging Base tag.
 * @param configParams     Map containing configuration parameters used to
 *                         initialize SBinet.  These parameters are the
 *                         properties defined above.  Once the function returns,
 *                         this Map can safely be destroyed as it is the
 *                         responsibility of SBinet to copy any data
 *                         it might require after initialization.
 *
 * @result VXIinet_RESULT_SUCCESS on success
 */
SBINET_API VXIinetResult SBinetInitEx(VXIlogInterface  *log,
                                      const VXIunsigned diagLogBase,
                                      const VXIMap     *configParams);

/**
 * Global platform shutdown of SBinet
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIinet_RESULT_SUCCESS on success
 */
SBINET_API VXIinetResult SBinetShutDown(VXIlogInterface *log);

/**
 * Create a new inet service handle
 *
 * @param log     [IN] VXI Logging interface used for error/diagnostic
 *                logging, must remain a valid pointer throughout the
 *                lifetime of the resource (until SBinetDestroyResource( )
 *                is called)
 * @param cache   [IN] VXI Cache interface used for HTTP document caching,
 *                must remain a valid pointer throughout the lifetime of
 *                the resource (until SBinetDestroyResource( ) is called)
 * @param inet    [IN/OUT] Will hold the created Inet resource.
 *
 * @result VXIinet_RESULT_SUCCESS on success
 */
SBINET_API
VXIinetResult SBinetCreateResource(VXIlogInterface    *log,
                                   VXIcacheInterface  *cache,
                                   VXIinetInterface   **inet);

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the logging interface passed to SBinetCreateResource()
 *  may be released as well.
 *
 * @param inet [IN/OUT] The inet resource created by SBinetCreateResource()
 *
 * @result VXIinet_RESULT_SUCCESS on success
 */
SBINET_API
VXIinetResult SBinetDestroyResource (VXIinetInterface **inet);

  /*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
