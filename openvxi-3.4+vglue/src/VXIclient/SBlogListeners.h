
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2007 Ampersand Inc., Doug Campbell
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

#ifndef _SBLOGLISTENERS_H              /* Allows multiple inclusions */
#define _SBLOGLISTENERS_H

#include "VXIheaderPrefix.h"
#ifdef SBLOGLISTENERS_DLL
#ifdef SBLOGLISTENERS_EXPORTS
#define SBLOGLISTENERS_API SYMBOL_EXPORT_DECL
#else
#define SBLOGLISTENERS_API SYMBOL_IMPORT_DECL
#endif
#else
#ifdef __cplusplus
#define SBLOGLISTENERS_API extern "C"
#else
#define SBLOGLISTENERS_API extern
#endif
#endif

#include "SBlog.h"

/**
 * @name SBlogListeners
 * @memo Reference implementation of SBlog listeners
 * @doc These functions are used to register and unregister for log
 * events. There are also convenience functions for enabling and
 * disabling tags. Finally four logging listener functions are defined.
 */

/*@{*/

/**
 * Log file formats, currently the Vocalocity proprietary log file
 * format with Latin 1 (ISO-8859-1) encoded characters or Unicode
 * (UTF-8) encoded characters.
 */
#define SBLOG_MIME_TEXT_LATIN1  L"text/plain;charset=ISO-8859-1"
#define SBLOG_MIME_TEXT_UTF8    L"text/plain;charset=UTF-8"
#define SBLOG_MIME_DEFAULT      SBLOG_MIME_TEXT_LATIN1

/**
 * Data that must be initialized then passed as the user data when
 * registering and unregistering SBlogListeners 
 * 
 * @param channel   Logical channel number for the resource
 * @param swirec    SWIrec (OpenSpeech Recognizer) interface, if non-NULL
 *                  events will be logged to the SWIrec event log as well
 *                  as to the SBlogListeners log file.
 */
typedef struct SBlogListenerChannelData {
  VXIint              channel;
} SBlogListenerChannelData;


/**
 * Global platform initialization of SBlogListeners
 *
 * @param  filename          Name of the file where diagnostic, error, and 
 *                           event information will be written. Pass NULL 
 *                           to disable logging to a file.
 * @param  fileMimeType      MIME content type for the log file, currently
 *                           SBLOG_MIME_TEXT_LATIN1 or SBLOG_MIME_TEXT_UTF8
 *                           as defined above, with SBLOG_MIME_TEXT_LATIN1
 *                           used if NULL or an empty string is passed.
 * @param  maxLogSizeBytes   Maximum size of the log file, in bytes. When
 *                           this size is exceeded, the log file is rotated,
 *                           with the existing file moved to a backup name
 *                           (<filename>.old) and a new log file started.
 * @param  contentDir        Directory to use for storing large (potentially
 *                           binary) content that is logged, such as VoiceXML
 *                           documents or waveforms
 * @param  maxContentDirSize Maximum size, in Bytes, of the content logging
 *                           directory (NOTE: currently not implemented, but
 *                           will be in a future release)
 * @param  logToStdout       TRUE to log diagnostic and error messages to
 *                           standard out, FALSE to disable. Event reports
 *                           are never logged to standard out.
 * @param  keepLogFileOpen   TRUE to keep the log file continuously open,
 *                           FALSE to close it when not logging. Performance
 *                           is much better if it is kept open, but doing
 *                           so prevents system operators from manually
 *                           rotating the log file when desired (simply
 *                           moving it to a new name from the command line,
 *                           with a new log file being automatically created).
 * @param  reportErrorText   TRUE to load the XML error mapping files as
 *                           passed in errorMapFiles at startup so that when
 *                           errors are logged the appropriate error text
 *                           from the XML error mapping file is displayed.
 *                           If FALSE, only the error number and module name
 *                           are displayed, with the system operator or
 *                           operations console software responsible for
 *                           the error text lookup. TRUE is best for
 *                           out-of-the-box developer use, while FALSE is
 *                           best for production use (minimizes error
 *                           reporting overhead and network bandwidth for
 *                           transporting errors to the remote console).
 * @param  errorMapFiles     Vector of paths to the XML error mapping files
 *                           that maps error numbers to text for a specific
 *                           component. Used when reportErrorText is TRUE,
 *                           see above.
 *
 * @result VXIlog_RESULT_SUCCESS on success
 */
SBLOGLISTENERS_API VXIlogResult SBlogListenerInit(
 const VXIchar   *filename,
 VXIint32         maxLogSizeBytes,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen
);

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx(
 const VXIchar   *filename,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
);

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx2(
 const VXIchar   *filename,
 const VXIchar   *fileMimeType,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout,
 VXIbool          keepLogFileOpen,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
);

SBLOGLISTENERS_API VXIlogResult SBlogListenerInitEx3(
 const VXIchar   *filename,
 const VXIchar   *fileMimeType,
 VXIint32         maxLogSizeBytes,
 const VXIchar   *logContentDir,
 VXIint32         maxContentDirSize,
 VXIbool          logToStdout, 
 VXIbool          keepLogFileOpen,
 VXIbool          logPerformanceCounter,
 VXIbool          reportErrorText,
 const VXIVector *errorMapFiles
);

/**
 * Global platform shutdown of SBlogListeners
 *
 * @result VXIlog_RESULT_SUCCESS on success
 */
SBLOGLISTENERS_API VXIlogResult SBlogListenerShutDown(void);


/*=========================================================================
 */


/**
 * SBlog listener for diagnostic logging
 *
 * Register this listener with SBlog to use the SBlogListener
 * implementation for diagnostic logging.
 *
 * @result VXIlog_RESULT_SUCCESS on success
 */
SBLOGLISTENERS_API void DiagnosticListener(
  SBlogInterface *pThis,
  VXIunsigned     tagID,
  const VXIchar  *subtag,
  time_t          timestamp,
  VXIunsigned     timestampMsec,
  const VXIchar  *printmsg,
  void           *userdata
);


/**
 * SBlog listener for error logging
 *
 * Register this listener with SBlog to use the SBlogListener
 * implementation for error logging.
 *
 */
SBLOGLISTENERS_API void ErrorListener(
  SBlogInterface   *pThis,
  const VXIchar    *moduleName,
  VXIunsigned       errorID,
  time_t            timestamp,
  VXIunsigned       timestampMsec,
  const VXIVector  *keys,
  const VXIVector  *values,
  void             *userdata
);


/**
 * SBlog listener for event logging
 *
 * Register this listener with SBlog to use the SBlogListener
 * implementation for event logging.
 *
 */
SBLOGLISTENERS_API void EventListener(
  SBlogInterface   *pThis,
  VXIunsigned       eventID,
  time_t            timestamp,
  VXIunsigned       timestampMsec,
  const VXIVector  *keys,
  const VXIVector  *values,
  void             *userdata
);


/**
 * SBlog listener for content logging
 *
 * Register this listener with SBlog to use the SBlogListener
 * implementation for content logging.
 *
 * @result VXIlog_RESULT_SUCCESS on success
 */
SBLOGLISTENERS_API VXIlogResult ContentListener(
  SBlogInterface   *pThis,
  const VXIchar    *moduleName,
  const VXIchar    *contentType,
  void             *userdata,
  VXIString       **logKey,
  VXIString       **logValue,
  SBlogStream     **stream
);


/*=========================================================================
 */


/**
 * Convenience function for enabling a diagnostic tag, identical
 * to calling SBlogInterface::ControlDiagnosticTag
 *
 * @param  tagID    [IN] Identifier that classifies a group of logically
 *                   associated diagnostic messages (usually from a single
 *                   software module) that are desirable to enable or
 *                   disable as a single unit. See the top of SBlog.h
 *                   for tagID allocation rules.
 * @param  state    [IN] Boolean flag to turn the tag on (TRUE) or
 *                   off (FALSE).
 * 
 * @return VXIlog_RESULT_SUCCESS on success
 */
SBLOGLISTENERS_API VXIlogResult ControlDiagnosticTag
(
  VXIlogInterface  *pThis,
  VXIunsigned       tagID,
  VXIbool           state
);


/**
 * Convenience functions for registering and unregistering an error
 * listener, identical to calling
 * SBlogInterface::RegisterErrorListener and
 * SBlogInterface::UnregisterErrorListener except the user data must
 * be a SBlogListener channel log data structure.
 *
 * @param alistener      [IN] The subscribing Listener 
 * @param channelData    [IN] User data that will be returned to the 
 *                        when notification occurs listener.
 *                        Note: the same listener may be 
 *                        registered multiple times, as long
 *                        as unique userdata is passed. In this
 *                        case, the listener will be called once
 *                        for each unique userdata.
 * 
 * @return VXIlog_RESULT_SUCCESS on success 
*/
SBLOGLISTENERS_API VXIlogResult RegisterErrorListener(
  VXIlogInterface           *pThis,
  SBlogErrorListener        *alistener,
  SBlogListenerChannelData  *channelData
);  

SBLOGLISTENERS_API VXIlogResult UnregisterErrorListener(
  VXIlogInterface           *pThis,
  SBlogErrorListener        *alistener,
  SBlogListenerChannelData  *channelData
);  


/**
 * Convenience functions for registering and unregistering a diagnostic
 * listener, identical to calling
 * SBlogInterface::RegisterDiagnosticListener and
 * SBlogInterface::UnregisterDiagnosticListener except the user data must
 * be a SBlogListener channel log data structure.
 *
 * @param alistener      [IN] The subscribing Listener 
 * @param channelData    [IN] User data that will be returned to the 
 *                        when notification occurs listener.
 *                        Note: the same listener may be 
 *                        registered multiple times, as long
 *                        as unique userdata is passed. In this
 *                        case, the listener will be called once
 *                        for each unique userdata.
 * 
 * @return VXIlog_RESULT_SUCCESS on success 
 */
SBLOGLISTENERS_API VXIlogResult RegisterDiagnosticListener(
  VXIlogInterface           *pThis,
  SBlogDiagnosticListener   *alistener,
  SBlogListenerChannelData  *channelData
);


SBLOGLISTENERS_API VXIlogResult UnregisterDiagnosticListener(
  VXIlogInterface           *pThis,
  SBlogDiagnosticListener   *alistener,
  SBlogListenerChannelData  *channelData
);


/**
 * Convenience functions for registering and unregistering an event
 * listener, identical to calling
 * SBlogInterface::RegisterEventListener and
 * SBlogInterface::UnregisterEventListener except the user data must
 * be a SBlogListener channel log data structure.
 *
 * @param alistener      [IN] The subscribing Listener 
 * @param channelData    [IN] User data that will be returned to the 
 *                        when notification occurs listener.
 *                        Note: the same listener may be 
 *                        registered multiple times, as long
 *                        as unique userdata is passed. In this
 *                        case, the listener will be called once
 *                        for each unique userdata.
 * 
 * @return VXIlog_RESULT_SUCCESS on success 
 */
SBLOGLISTENERS_API VXIlogResult RegisterEventListener(
  VXIlogInterface           *pThis,
  SBlogEventListener        *alistener,
  SBlogListenerChannelData  *channelData
);  
 
SBLOGLISTENERS_API VXIlogResult UnregisterEventListener(
  VXIlogInterface           *pThis,
  SBlogEventListener        *alistener,
  SBlogListenerChannelData  *channelData
);


/**
 * Convenience functions for registering and unregistering a content
 * listener, identical to calling
 * SBlogInterface::RegisterContentListener and
 * SBlogInterface::UnregisterContentListener except the user data must
 * be a SBlogListener channel log data structure.
 *
 * @param alistener      [IN] The subscribing Listener 
 * @param channelData    [IN] User data that will be returned to the 
 *                        when notification occurs listener.
 *                        Note: the same listener may be 
 *                        registered multiple times, as long
 *                        as unique userdata is passed. In this
 *                        case, the listener will be called once
 *                        for each unique userdata.
 * 
 * @return VXIlog_RESULT_SUCCESS on success 
 */
SBLOGLISTENERS_API VXIlogResult RegisterContentListener(
  VXIlogInterface           *pThis,
  SBlogContentListener      *alistener,
  SBlogListenerChannelData  *channelData
);  
 
SBLOGLISTENERS_API VXIlogResult UnregisterContentListener(
  VXIlogInterface           *pThis,
  SBlogContentListener      *alistener,
  SBlogListenerChannelData  *channelData
);

/*@}*/
#include "VXIheaderSuffix.h"

#endif  /* include guard */
