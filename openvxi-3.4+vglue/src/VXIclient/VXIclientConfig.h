
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

#ifndef _VXICLIENT_CONFIG_H
#define _VXICLIENT_CONFIG_H

/**
  @name VXIclient properties
  @memo Definition of properties used by testClient
  @version 1.0 <br>
  @doc Collection of properties for each of the components used in the
  OpenSpeech Browser.  These properites are used by testClient and are
  read from the configuration file.
*/

/*@{*/

#define CLIENT_CURRENT_NUMCALL         L"client.current.numCall"

/**
  @name Recognizer implementation keys
  @memo Keys for the Recognizer implementation that effects dynamic library
 */
/*@{*/
#define CLIENT_REC_IMPLEMENTATION         L"client.rec.implementation"
/*@}*/

/**
  @name Prompt implementation keys
  @memo Keys for the Prompt implementation that effects dynamic library
 */
/*@{*/
#define CLIENT_PROMPT_IMPLEMENTATION      L"client.prompt.implementation"
/*@}*/

/**
  @name Tel. implementation keys
  @memo Keys for the Tel. implementation that effects dynamic library
 */
/*@{*/
#define CLIENT_TEL_IMPLEMENTATION         L"client.tel.implementation"
/*@}*/

/**
  @name TRD keys
  @memo Keys for the TRD utilities library configuration
 */
/*@{*/
#define CLIENT_TRD_THREAD_STACKSIZE       L"client.trd.threadStackSize"
/*@}*/

/**
  @name INET keys
  @memo Keys for the INET configuration 
  @version 1.0
  @doc Properties read from the configuration file used to
  configure SBinet.
 */
/*@{*/
#define CLIENT_INET_PROXY_SERVER               L"client.inet.proxyServer"
#define CLIENT_INET_PROXY_PORT                 L"client.inet.proxyPort"
#define CLIENT_INET_PROXY_RULES                L"client.inet.proxyRules"
#define CLIENT_INET_PROXY_RULE_PREFIX          L"client.inet.proxyRule."
#define CLIENT_INET_EXTENSION_RULES            L"client.inet.extensionRules"
#define CLIENT_INET_EXTENSION_RULE_PREFIX      L"client.inet.extensionRule."
#define CLIENT_INET_USER_AGENT                 L"client.inet.userAgent"
#define CLIENT_INET_ACCEPT_COOKIES             L"client.inet.acceptCookies"
/*@}*/

#define OSR_USE_INTERNAL_SBINET                L"com.vocalocity.osr.use.sbinet"

/**
 @name Cache Keys
 @memo Keys for the Cache configuration
 */
/*@{*/
#define CLIENT_CACHE_CACHE_DIR                 L"client.cache.cacheDir"
#define CLIENT_CACHE_CACHE_TOTAL_SIZE_MB       L"client.cache.cacheTotalSizeMB"
#define CLIENT_CACHE_CACHE_ENTRY_MAX_SIZE_MB   L"client.cache.cacheEntryMaxSizeMB"
#define CLIENT_CACHE_CACHE_ENTRY_EXP_TIME_SEC  L"client.cache.cacheEntryExpTimeSec"
#define CLIENT_CACHE_UNLOCK_ENTRIES            L"client.cache.unlockEntries"
#define CLIENT_CACHE_CACHE_LOW_WATER_MARK_MB   L"client.cache.cacheLowWaterMarkMB"
/*@}*/

/**
  @name JSI Keys
  @memo Keys for the ECMAScript interface
 */
/*@{*/
#define CLIENT_JSI_RUNTIME_SIZE_BYTES          L"client.jsi.runtimeSizeBytes"
#define CLIENT_JSI_CONTEXT_SIZE_BYTES          L"client.jsi.contextSizeBytes"
#define CLIENT_JSI_MAX_BRANCHES                L"client.jsi.maxBranches"
#define CLIENT_JSI_GLOBAL_SCRIPT_FILE          L"client.jsi.globalScriptFile"
/*@}*/

/**
  @name Prompt Keys
  @memo Keys for the prompt interface
 */
/*@{*/
#define CLIENT_PROMPT_RESOURCES                L"client.prompt.resources"
#define CLIENT_PROMPT_RESOURCE_PREFIX          L"client.prompt.resource."
#define CLIENT_PROMPT_CACHE                    L"client.prompt.enableCache"
/*@}*/

/**
  @name Recognizer Keys
  @memo Keys for the recognition interface
 */
/*@{*/
#define CLIENT_REC_INIT_URI                    L"client.rec.initURI"
/*@}*/

/**
  @name VXI keys
  @memo Keys for the VXI interface
  */
/*@{*/
#define CLIENT_VXI_BEEP_URI                    L"client.vxi.beepURI"
#define CLIENT_VXI_DEFAULTS_URI                L"client.vxi.defaultsURI"
/*@}*/

/**
  @name Log keys
  @memo Keys for the Log interface
 */
/*@{*/
#define CLIENT_LOG_FILE_NAME                L"client.log.filename"
#define CLIENT_LOG_FILE_MIME_TYPE           L"client.log.fileMimeType"
#define CLIENT_LOG_MAX_SIZE_MB              L"client.log.maxLogSizeMB"
#define CLIENT_LOG_CONTENT_DIR              L"client.log.contentDir"
#define CLIENT_LOG_CONTENT_TOTAL_SIZE_MB    L"client.log.contentTotalSizeMB"
#define CLIENT_LOG_DIAG_TAG_KEY_PREFIX      L"client.log.diagTag."
#define CLIENT_LOG_LOG_TO_STDOUT            L"client.log.logToStdout"
#define CLIENT_LOG_KEEP_LOG_FILE_OPEN       L"client.log.keepLogFileOpen"
#define CLIENT_LOG_REPORT_ERROR_TEXT        L"client.log.reportErrorText"
#define CLIENT_LOG_ERROR_MAP_FILES          L"client.log.errorMapFiles"
#define CLIENT_LOG_ERROR_MAP_FILE_PREFIX    L"client.log.errorMapFile."
/*@}*/

/**
  @name session connection keys
  @memo Keys for the session connection
 */
/*@{*/
#define CLIENT_SESSION_CONNECTION_LOCAL_URI         L"client.session.connection.local.uri"
#define CLIENT_SESSION_CONNECTION_REMOTE_URI        L"client.session.connection.remote.uri"
#define CLIENT_SESSION_CONNECTION_PROTOCOL_NAME     L"client.session.connection.protocol.name"
#define CLIENT_SESSION_CONNECTION_PROTOCOL_VERSION  L"client.session.connection.protocol.version"
#define CLIENT_SESSION_CONNECTION_AAI               L"client.session.connection.aai"
#define CLIENT_SESSION_CONNECTION_ORIGINATOR        L"client.session.connection.originator"
#define CLIENT_SESSION_CONNECTION_REDIRECT_DOT      L"client.session.connection.redirect."
/*@}*/

/**
  @name Diagnostic Tag keys
  @memo Base diagnostic tag offset for each interface.
  @doc The value for these keys controls the tags numbers that must be
  turned on to get diagnostic output.
 */
/*@{*/
#define CLIENT_HW_DIAG_BASE                 L"client.hw.diagLogBase"
#define CLIENT_AUDIO_DIAG_BASE              L"client.audio.diagLogBase"
#define CLIENT_CACHE_DIAG_BASE              L"client.cache.diagLogBase"
#define CLIENT_INET_DIAG_BASE               L"client.inet.diagLogBase"
#define CLIENT_JSI_DIAG_BASE                L"client.jsi.diagLogBase"
#define CLIENT_OBJ_DIAG_BASE                L"client.object.diagLogBase"
#define CLIENT_PROMPT_DIAG_BASE             L"client.prompt.diagLogBase"
#define CLIENT_REC_DIAG_BASE                L"client.rec.diagLogBase"
#define CLIENT_TEL_DIAG_BASE                L"client.tel.diagLogBase"
#define CLIENT_VXI_DIAG_BASE                L"client.vxi.diagLogBase"
#define CLIENT_CLIENT_DIAG_BASE             L"client.client.diagLogBase"
/*@}*/



#endif /* include guard */
