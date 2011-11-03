
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

#ifndef _VXIPROMPT_H
#define _VXIPROMPT_H

#include "VXItypes.h"
#include "VXIvalue.h"

#include "VXIheaderPrefix.h"
#ifdef VXIPROMPT_EXPORTS
#define VXIPROMPT_API SYMBOL_EXPORT_DECL
#else
#define VXIPROMPT_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup VXIprompt Prompt Interface
 *
 * Abstract interface for Prompting functionality.  Prompts are represented
 * as a series of SSML documents.<p>
 */
/*@{*/

/**
 * @name Properties
 * Keys identifying properties in a VXIMap passed to the Prompt
 * interface functions.
 */
/*@{*/
/**
 * Key to a VXIMap that holds audio content created during a 
 * &lt;record&gt;.  This map will contain key matching &lt;mark&gt; names
 * in a SSML document passed to Queue.  Each key will return
 * a VXIContent value.  See Queue for more information.
 */
#define PROMPT_AUDIO_REFS            L"vxi.prompt.audioReferences"

/**
 * Key to a VXIInteger that indicates whether a SSML document passed to
 * Prefetch should be prefetched.  The value is 0 if no prefetch is
 * requested.  1 if prefetch is desired.
 */
#define PROMPT_PREFETCH_REQUEST      L"vxi.prompt.prefetchLevel"
/*@}*/

/**
 * For audio created during a &lt;record&gt;, the SSML document passed
 * to Queue will contain a &lt;mark&gt; with a name prefixed with this
 * string.  See Queue for more information.
 */
#define PROMPT_AUDIO_REFS_SCHEME     L"x-vxiprompt-ref"


/**
 * Result codes for interface methods
 *
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues)
 */
typedef enum VXIpromptResult {
  /* Fatal error, terminate call     */
  VXIprompt_RESULT_FATAL_ERROR       =  -100,
  /* I/O error                       */
  VXIprompt_RESULT_IO_ERROR           =   -8,
  /* Out of memory                   */
  VXIprompt_RESULT_OUT_OF_MEMORY      =   -7,
  /* System error, out of service    */
  VXIprompt_RESULT_SYSTEM_ERROR       =   -6,
  /* Errors from platform services   */
  VXIprompt_RESULT_PLATFORM_ERROR     =   -5,
  /* Return buffer too small         */
  VXIprompt_RESULT_BUFFER_TOO_SMALL   =   -4,
  /* Property name is not valid      */
  VXIprompt_RESULT_INVALID_PROP_NAME  =   -3,
  /* Property value is not valid     */
  VXIprompt_RESULT_INVALID_PROP_VALUE =   -2,
  /* Invalid function argument       */
  VXIprompt_RESULT_INVALID_ARGUMENT   =   -1,
  /* Success                         */
  VXIprompt_RESULT_SUCCESS            =    0,
  /* Normal failure, nothing logged  */
  VXIprompt_RESULT_FAILURE            =    1,
  /* Non-fatal non-specific error    */
  VXIprompt_RESULT_NON_FATAL_ERROR    =    2,
  /* URI fetch timeout               */
  VXIprompt_RESULT_FETCH_TIMEOUT      =   50,
  /* URI fetch error                 */
  VXIprompt_RESULT_FETCH_ERROR        =   51,
  /* Bad sayas class                 */
  VXIprompt_RESULT_BAD_SAYAS_CLASS    =   52,
  /* TTS access failure              */
  VXIprompt_RESULT_TTS_ACCESS_ERROR   =   53,
  /* TTS unsupported language        */
  VXIprompt_RESULT_TTS_BAD_LANGUAGE   =   54,
  /* TTS unsupported document type   */
  VXIprompt_RESULT_TTS_BAD_DOCUMENT   =   55,
  /* TTS document syntax error       */
  VXIprompt_RESULT_TTS_SYNTAX_ERROR   =   56,
  /* TTS generic error               */
  VXIprompt_RESULT_TTS_ERROR          =   57,
  /* Resource busy, such as TTS      */
  VXIprompt_RESULT_RESOURCE_BUSY      =   58,
  /* HW player unsupported MIME type */
  VXIprompt_RESULT_HW_BAD_TYPE        =   59,
  /* Generic HW player error         */
  VXIprompt_RESULT_HW_ERROR           =   60,
  /* resource is not avaialable      */
  VXIprompt_RESULT_NO_RESOURCE        =   61,
  /* resource is not authorized      */
  VXIprompt_RESULT_NO_AUTHORIZATION   =   62,
  /* Operation is not supported      */
  VXIprompt_RESULT_UNSUPPORTED        =  100
} VXIpromptResult;


/**
 * Abstract interface for Prompting functionality.  Prompts are represented
 * as a series of SSML documents.<p>
 *
 * The Prompt interface the handles prefetching, caching, and
 * streaming audio as required to provide good response times and low
 * CPU and network overhead. <p>
 *
 * There is one prompt interface per thread/line.
 */
typedef struct VXIpromptInterface {
  /**
   * Get the VXI interface version implemented
   *
   * @return  VXIint32 for the version number. The high high word is
   *          the major version number, the low word is the minor version
   *          number, using the native CPU/OS byte order. The current
   *          version is VXI_CURRENT_VERSION as defined in VXItypes.h.
   */
  VXIint32 (*GetVersion)(void);

  /**
   * Get the name of the implementation
   *
   * @return  Implementation defined string that must be different from
   *          all other implementations. The recommended name is one
   *          where the interface name is prefixed by the implementator's
   *          Internet address in reverse order, such as com.xyz.rec for
   *          VXIrec from xyz.com. This is similar to how VoiceXML 1.0
   *          recommends defining application specific error types.
   */
  const VXIchar* (*GetImplementationName)(void);

  /**
   * Reset for a new session.
   *
   * This must be called for each new session, allowing for
   * call specific handling to occur. For some implementations, this
   * can be a no-op.  For others runtime binding of resources or other
   * call start specific handling may be done on this call.<p>
   *
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                    arguments for the new session
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*BeginSession)(struct VXIpromptInterface  *pThis,
                                  VXIMap                     *args);

  /**
   * Performs cleanup at the end of a call session.
   *
   * This must be called at the termination of a call, allowing for
   * call specific termination to occur. For some implementations, this
   * can be a no-op. For others runtime resources may be released or
   * other adaptation may be completed.<p>
   *
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                    arguments for ending the session
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*EndSession)(struct VXIpromptInterface  *pThis,
                                VXIMap                     *args);

  /**
   * Start playing queued segments (non-blocking).
   *
   * Segments queued after this is called will not be played until
   * this is called again.  It is possible errors may occur after this
   * function has returned: Wait() will return the appropriate error
   * if one occurred.<p>
   *
   * Note that this stops the current PlayFiller() operation although
   * possibly after some delay, see PlayFiller() for more information.<p>
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*Play)(struct VXIpromptInterface  *pThis);

  /**
   * Queues and possibly starts the special play of a filler segment, 
   * non-blocking
   * 
   * This plays a standard segment in a special manner in order to
   * satisfy "filler" needs. A typical example is the VoiceXML
   * fetchaudio attribute, used to specify filler audio that gets
   * played while a document fetch is being performed and then
   * interrupted once the fetch completes.<p>
   *
   * The filler segment is played to the caller once all active Play()
   * operations, if any, have completed. If Play(), Wait(), or
   * PlayFiller() is called before the filler segment starts playing
   * it is cancelled and never played. If one of those functions is
   * instead called after the filler segment starts playing, the
   * filler segment is stopped once the minimum playback duration
   * expires.<p>
   *
   * NOTE: this does not trigger the play of segments that have been queued
   * but not yet played via Play().
   *
   * @param type        <b>[IN]</b> Type of segment, either a MIME content type,
   *                    a sayas class name, or NULL to automatically detect
   *                    a MIME content type (only valid when src is
   *                    non-NULL). The supported MIME content types and
   *                    sayas class names are implementation dependant.
   * @param src         <b>deprecated</b>
   * @param text        <b>[IN]</b> Text (possibly with markup) to play via TTS
   *                    or sayas classes, pass NULL when src is non-NULL.
   *                    The format of text for TTS playback may be W3C
   *                    SSML (type set to VXI_MIME_SSML) or simple wchar_t
   *                    text (type set to VXI_MIME_UNICODE_TEXT).  The
   *                    implementation may also support other formats.
   * @param properties  <b>[IN]</b> Properties to control the fetch, queue, and
   *                    play, as specified above. May be NULL.
   * @param minPlayMsec <b>[IN]</b> Minimum playback duration for the
   *                    filler prompt once it starts playing, in
   *                    milliseconds. This is used to "lock in" a play
   *                    so that no less then this amount of audio is
   *                    heard by the caller once it starts playing,
   *                    avoiding confusion from audio that is played
   *                    for an extremely brief duration and then cut
   *                    off. Note that the filler prompt may never
   *                    be played at all, however, if cancelled before
   *                    it ever starts playing as described above.
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*PlayFiller)(struct VXIpromptInterface *pThis,
                                const VXIchar             *type,
                                const VXIchar             *src,
                                const VXIchar             *text,
                                const VXIMap              *properties,
                                VXIlong                    minPlayMsec);

  /**
   * Prefetch a segment (non-blocking).
   *
   * This fetches the segment in the background, since this returns
   * before the fetch proceeds failures during the fetch will not be
   * reported (invalid URI, missing file, etc.).  This may be called 
   * prior to Queue() (possibily multiple times with increasing VXIinet
   * prefetch priorities as the time for playback gets closer).
   *
   * @param type        <b>[IN]</b> Type of segment.  Currently, the only type
   *                                used is VXI_MIME_SSML.
   * @param src         <b>deprecated</b>
   * @param text        <b>[IN]</b> The prompt text.  Currently, the only text
   *                                format used is SSML.
   * @param properties  <b>[IN]</b> Properties to control the fetch, queue, and
   *                                play, as specified above. May be NULL.
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*Prefetch)(struct VXIpromptInterface *pThis,
                              const VXIchar             *type,
                              const VXIchar             *src,  /* deprecated - NOT USED */
                              const VXIchar             *text,
                              const VXIMap              *properties);

  /**
   * Queue a segment for playing (non-blocking).
   * 
   * The segment does not start playing until the Play() method is
   * called.<p>
   *
   * VoiceXML allows audio collected during the &lt;record&gt; element to be played
   * in the middle of SSML documents.  These recordings are passed by 
   * VXI to the VXIprompt implementation inside the properties VXIMap.
   * The PROMPT_AUDIO_REFS points to a second VXIMap containing pairs of
   * identifiers (VXIStrings) and their associated recording (VXIContent).<p>
   *
   * Within the SSML document, the audio recording is replaced by a mark
   * element whose 'name' attribute is a key inside the PROMPT_AUDIO_REFS
   * map.  i.e., Use the mark name as the key to access the VXIContent
   * in the PROMPT_AUDIO_REFS map.  The mark name for audio content will
   * be prefixed with PROMPT_AUDIO_REFS_SCHEME so that it can be distinguished
   * from other &lt;mark&gt; elements.<p>
   *
   * @param type        <b>[IN]</b> Type of segment.  Currently, the only type
   *                    used is VXI_MIME_SSML.
   * @param src         <b>deprecated</b>
   * @param text        <b>[IN]</b> The prompt text.  Currently, the only text
   *                    format used is SSML.
   * @param properties  <b>[IN]</b> Properties to control the fetch, queue, and
   *                    play, as specified above. May be NULL.
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*Queue)(struct VXIpromptInterface *pThis,
                           const VXIchar             *type,
                           const VXIchar             *src, /* deprecated - NOT USED */
                           const VXIchar             *text,
                           const VXIMap              *properties);

  /**
   * Wait until all played segments finish playing, blocking
   *
   * Note that this stops the current PlayFiller() operation although
   * possibly after some delay, see PlayFiller() for more information.<p>
   *
   * If Wait is called, and no audio is playing, this function should
   * not block.<p>
   *
   * @param playResult <b>[OUT]</b> Most severe error code resulting from
   *                   a Play() operation since the last Wait()
   *                   call, since Play() is asynchronous, errors
   *                   may have occurred after one or more calls to
   *                   it have returned. Note that this ignores any
   *                   errors resulting from PlayFiller() operations.
   *
   * @return VXIprompt_RESULT_SUCCESS on success
   */
  VXIpromptResult (*Wait)(struct VXIpromptInterface *pThis,
                          VXIpromptResult           *playResult);
} VXIpromptInterface;

/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
