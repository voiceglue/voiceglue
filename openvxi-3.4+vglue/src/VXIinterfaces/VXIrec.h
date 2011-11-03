
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

#ifndef _VXIREC_H
#define _VXIREC_H

#include "VXItypes.h"
#include "VXIvalue.h"
#include "VXItel.h"

#include "VXIheaderPrefix.h"
#ifdef VXIREC_EXPORTS
#define VXIREC_API SYMBOL_EXPORT_DECL
#else
#define VXIREC_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
struct VXIrecGrammar;
#else
typedef struct VXIrecGrammar { void * dummy; } VXIrecGrammar;
#endif

/**
 * \defgroup VXIrec Rec Interface
 *
 * Abstract interface for recognition functionality required by
 * VoiceXML. Recognition is performed against VXIrecGrammars, abstract
 * grammar types managed by the interface. The exact grammar formats
 * handled are implementation dependant. <p>
 */
/*@{*/

#define VXIREC_MIMETYPE_ULAW       VXI_MIME_ULAW
#define VXIREC_MIMETYPE_XMLRESULT  L"application/x-vocalocity.nlsml"

/**
 * @name Properties
 * Keys identifying properties in VXIMap
 *
 * For VXIrecInterface::LoadGrammarURI and VXIrecInterface::LoadGrammarString, 
 * the only standard properties are VXIinet properties that are simply passed 
 * through to VXIinet for fetches as-is. The implementation may define additional
 * properties. <p>
 *
 * For VXIrecInterface::Recognize and VXIrecInterface::Record, the following 
 * standard properties are defined:
 */
/*@{*/
#define REC_BARGEIN_TYPE            L"vxi.rec.bargeinType"         /* VXIInteger, VXIrecBargeinType */
#define REC_BEEP                    L"vxi.rec.beep"
#define REC_CONFIDENCE_LEVEL        L"vxi.rec.confidenceLevel"     /* VXIFloat 0.0 - 1.0 */
#define REC_DTMF_FLUSH_QUEUE        L"vxi.rec.flushDTMFQueue"      /* VXIBoolean */
#define REC_DTMF_TERMINATOR_CHAR    L"vxi.rec.terminatorChar"      /* VXIString with one character */
#define REC_DTMF_TIMEOUT            L"vxi.rec.timeout"             /* VXIInteger */
#define REC_DTMF_TIMEOUT_INTERDIGIT L"vxi.rec.timeoutInterdigit"   /* VXIInteger */
#define REC_DTMF_TIMEOUT_TERMINATOR L"vxi.rec.timeoutTerminator"   /* VXIInteger */
#define REC_INPUT_MODES             L"vxi.rec.inputModes"          /* VXIInteger, VXIrecInputMode value */
#define REC_MAX_RECORDING_TIME      L"vxi.rec.maxRecordingTime"    /* VXIInteger */
#define REC_RECORD_MIME_TYPE        L"vxi.rec.recordingMimeType"   /* VXIString */
#define REC_RECORDUTTERANCE         L"vxi.rec.recordutterance"     /* VXIString ('true' | 'false') */
#define REC_RECORDUTTERANCETYPE     L"vxi.rec.recordutterancetype" /* VXIString */
#define REC_RESULT_NBEST_SIZE       L"vxi.rec.resultNbestSize"     /* VXIInteger */
#define REC_SENSITIVITY             L"vxi.rec.sensitivity"         /* VXIFloat 0.0 - 1.0 */
#define REC_SPEED_VS_ACCURACY       L"vxi.rec.speedVsAccuracy"     /* VXIFloat 0.0-1.0 */
#define REC_TERMINATED_ON_DTMF      L"vxi.rec.terminatedOnDTMF"    /* VXIInteger, 0 for disabled, 1 for enabled */
#define REC_TIMEOUT                 L"vxi.rec.timeout"             /* VXIInteger */
#define REC_TIMEOUT_COMPLETE        L"vxi.rec.timeoutComplete"     /* VXIInteger */
#define REC_TIMEOUT_INCOMPLETE      L"vxi.rec.timeoutIncomplete"   /* VXIInteger */
#define REC_TIMEOUT_SPEECH          L"vxi.rec.maxspeechtime"       /* VXIInteger */
/*@}*/

/**
 * @name Grammar Properties
 *
 * For VXIrecInterface::LoadGrammarURI, VXIrecInterface::LoadGrammarString, and 
 * VXIrecInterface::LoadGrammarOption, the following standard properties are defined:
 */
/*@{*/
#define REC_GRAMMAR_ACCEPTANCE      L"vxi.rec.grammarAcceptance" /* VXIInteger, 0 or undefined for exact, 1 for approximate */
#define REC_GRAMMAR_MODE            L"vxi.rec.grammarMode"       /* VXIString ('voice' | 'dtmf' */
#define REC_GRAMMAR_WEIGHT          L"vxi.rec.grammarWeight"     /* VXIFloat */
#define REC_LANGUAGE                L"vxi.rec.language"          /* VXIString */
#define REC_PREFETCH_REQUEST        L"vxi.rec.prefetchLevel"     /* VXIInteger, 0 if no prefetch is requested, 1 is prefetch is desired */
/*@}*/

/**
 * Bargein types as set in the REC_BARGEIN_TYPE property defined above
 */
typedef enum VXIrecBargeinType {
  REC_BARGEIN_SPEECH          = 2,  /** Terminate prompt when start of speech detected */
  REC_BARGEIN_HOTWORD         = 4   /** Terminate prompt on recognition success        */
} VXIrecBargeinType;

/**
 * Input modes as set in the REC_INPUT_MODE property defined above and
 * as returned in VXIrecResult structures
 */
typedef enum VXIrecInputMode {
  REC_INPUT_MODE_DTMF         = 2,  /** Only DTMF input accepted       */
  REC_INPUT_MODE_SPEECH       = 4,  /** Only speech input accepted     */
  REC_INPUT_MODE_DTMF_SPEECH  = 6   /** DTMF and speech input accepted */
} VXIrecInputMode;

/**
 * MIME content types for LoadGrammarURI and LoadGrammarString,
 * the implementation usually supports additional MIME types 
 */
#define REC_MIME_CHOICE         L"text/x-grammar-choice"
#define REC_MIME_CHOICE_DTMF    L"text/x-grammar-choice-dtmf"

/**
 * Recognition result as returned by Recognize().
 */
typedef struct VXIrecRecognitionResult {
  /**
   * The semantic interpretation of the recognition expressed using the
   * Natural Language Semantic Markup Language (NLSML).  The mimetype
   * of the VXIContent may allow multiple formats to be supported.
   */
  VXIContent*     xmlresult;

  /**
   * Waveform of the caller's utterance.
   *
   * If the VXML property 'recordutterance' is true, the audio data
   * of the callers utterance should be placed here.  Otherwise,
   * this field should be set to NULL.
   *
   * The MIME type of the utterance, as specified by 'recordutterancetype'
   * should be used when calling VXIContentCreate.
   */
  VXIContent*     utterance;

  /**
   * Utterance length in milliseconds.  This field is ignored if
   * 'utterance' is NULL.
   */
  VXIunsigned     utteranceDuration;

  /**
   * The name of the mark last executed by the SSML processor before 
   * barge-in occurred or the end of audio playback occurred. If no 
   * mark was executed, this variable is NULL.
   */
  VXIString*      markname;

  /**
   * The number of milliseconds that elapsed since the last mark was 
   * executed by the SSML processor until barge-in occurred or the end 
   * of audio playback occurred. If no mark was executed, this variable 
   * is NULL.
   */
  VXIunsigned     marktime;

  /**
   * Call to release the resources stored with this result structure.
   * The implementor of this function is expected to destroy all members
   * of this structure.  i.e., VXIContentDestory, VXIStringDestroy,
   * or VXIULongDestroy respectively.
   */
  void            (*Destroy)(struct VXIrecRecognitionResult **result);
} VXIrecRecognitionResult;


/**
 * Record results structure as returned by Record()
 */
typedef struct VXIrecRecordResult {
  /** 
   * The bytes of the recording 
   */
  VXIContent * waveform;

  /**
   * The semantic interpretation of the recognition expressed using the
   * Natural Language Semantic Markup Language (NLSML).  The mimetype
   * of the VXIContent may allow multiple formats to be supported.
   */
  VXIContent*  xmlresult;

  /**
   * The name of the mark last executed by the SSML processor before 
   * barge-in occurred or the end of audio playback occurred. If no 
   * mark was executed, this variable is NULL.
   */
  VXIString*      markname;

  /**
   * The number of milliseconds that elapsed since the last mark was 
   * executed by the SSML processor until barge-in occurred or the end 
   * of audio playback occurred. If no mark was executed, this variable 
   * is NULL.
   */
  VXIunsigned     marktime;

  /** 
   * Duration of the recording in milliseconds 
   */
  VXIunsigned duration;

  /** 
   * DTMF character which terminated the recording, or 0 if none. 
   */
  VXIbyte termchar;

  /** 
   * True if the recording was terminated because the maximum time 
   * limit was reached 
   */
  VXIbool maxtime;

  /**
   * Call to release the resources stored with this result structure.
   * The implementor of this function is expected to destroy all members
   * of this structure.  i.e., VXIContentDestory, VXIStringDestroy,
   * or VXIULongDestroy respectively.
   */
  void (*Destroy)(struct VXIrecRecordResult **result);
} VXIrecRecordResult;


/**
 * Record results structure as returned by HotwordTransfer()
 */
typedef struct VXIrecTransferResult {
  /**
   * The semantic interpretation of the recognition expressed using the
   * Natural Language Semantic Markup Language (NLSML).  The mimetype
   * of the VXIContent may allow multiple formats to be supported.
   */
  VXIContent*  xmlresult;

  /** 
   * Duration of the transfer in milliseconds
   */
  VXIunsigned duration;

  /**
   * Result code for the transfer request 
   */
  VXItelTransferStatus status;

  /**
   * The name of the mark last executed by the SSML processor before 
   * barge-in occurred or the end of audio playback occurred. If no 
   * mark was executed, this variable is NULL.
   */
  VXIString*      markname;

  /**
   * The number of milliseconds that elapsed since the last mark was 
   * executed by the SSML processor until barge-in occurred or the end 
   * of audio playback occurred. If no mark was executed, this variable 
   * is NULL.
   */
  VXIunsigned     marktime;

  /**
   * Waveform of the caller's utterance.
   *
   * If the VXML property 'recordutterance' is true, the audio data
   * of the callers utterance should be placed here.  Otherwise,
   * this field should be set to NULL.
   *
   * The MIME type of the utterance, as specified by 'recordutterancetype'
   * should be used when calling VXIContentCreate.
   */
  VXIContent*     utterance;

  /**
   * Utterance length in milliseconds.  This field is ignored if
   * 'utterance' is NULL.
   */
  VXIunsigned     utteranceDuration;

  /**
   * Call to release the resources stored with this result structure.
   * The implementor of this function is expected to destroy all members
   * of this structure.  i.e., VXIContentDestory, VXIStringDestroy,
   * or VXIULongDestroy respectively.
   */
  void            (*Destroy)(struct VXIrecTransferResult **result);
} VXIrecTransferResult;


/**
 * Result codes for interface methods
 * 
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues) 
 */
typedef enum VXIrecResult {
  /* Fatal error, terminate call     */
  VXIrec_RESULT_FATAL_ERROR        = -100, 
  /* I/O error                       */
  VXIrec_RESULT_IO_ERROR           =   -8,
  /* Out of memory                   */
  VXIrec_RESULT_OUT_OF_MEMORY      =   -7, 
  /* System error, out of service    */
  VXIrec_RESULT_SYSTEM_ERROR       =   -6, 
  /* Errors from platform services   */
  VXIrec_RESULT_PLATFORM_ERROR     =   -5, 
  /* Return buffer too small         */
  VXIrec_RESULT_BUFFER_TOO_SMALL   =   -4, 
  /* Property name is not valid      */
  VXIrec_RESULT_INVALID_PROP_NAME  =   -3, 
  /* Property value is not valid     */
  VXIrec_RESULT_INVALID_PROP_VALUE =   -2, 
  /* Invalid function argument       */
  VXIrec_RESULT_INVALID_ARGUMENT   =   -1, 
  /* Success                         */
  VXIrec_RESULT_SUCCESS            =    0,
  /* Normal failure, nothing logged  */
  VXIrec_RESULT_FAILURE            =    1,
  /* Non-fatal non-specific error    */
  VXIrec_RESULT_NON_FATAL_ERROR    =    2, 
  /* URL fetch timeout               */
  VXIrec_RESULT_FETCH_TIMEOUT      =   50,
  /* URL fetch error                 */
  VXIrec_RESULT_FETCH_ERROR        =   51,
  /* Unsupported grammar type        */
  VXIrec_RESULT_BAD_GRAMMAR        =   52,
  /* Grammar syntax error            */
  VXIrec_RESULT_SYNTAX_ERROR       =   53,
  /* Unsupported MIME type           */
  VXIrec_RESULT_BAD_MIME_TYPE      =   54,
  VXIrec_RESULT_UNSUPPORTED_FORMAT =   VXIrec_RESULT_BAD_MIME_TYPE,
  /* Unsupported builtin		 */
  VXIrec_RESULT_UNSUPPORTED_BUILTIN =   55,
  /* Unsupported language            */
  VXIrec_RESULT_UNSUPPORTED_LANGUAGE =   56,
  /* Requested resource not available */
  VXIrec_RESULT_NO_RESOURCE        =   57,
  /* Operation is not authorized	 */
  VXIrec_RESULT_NO_AUTHORIZATION   =   58,
  /* User input is too long, exceeding */
  /* 'maxspeechtimeout' property 	 */
  VXIrec_RESULT_MAX_SPEECH_TIMEOUT =   59,

  /* HOT WORD TRANSFER RETURN CODES  */

  /* Call is not allowed to the destination */
  VXIrec_RESULT_CONNECTION_NO_AUTHORIZATION =   60,
  /* The destination URI is malformed */
  VXIrec_RESULT_CONNECTION_BAD_DESTINATION =   61,    
  /* The platform is not able to 	 */
  /* place a call to the destination */
  VXIrec_RESULT_CONNECTION_NO_ROUTE =   62,
  /* The platform cannot allocate    */
  /* resources to place the call     */
  VXIrec_RESULT_CONNECTION_NO_RESOURCE =   63,
  /* The platform does not support   */
  /* the URI format			 */
  VXIrec_RESULT_UNSUPPORTED_URI    =   64,
  /* Operation is not supported      */
  VXIrec_RESULT_UNSUPPORTED        =  100
} VXIrecResult;


/** 
 * Abstract interface for recognition functionality required by
 * VoiceXML. Recognition is performed against VXIrecGrammars, abstract
 * grammar types managed by the interface. The exact grammar formats
 * handled are implementation dependant. <p>
 *
 * If an asynchronous problem/error occurs in the platform's
 * recognizer, the VXI is notified through the VXIrecResult code for
 * the current or next function call. <p>
 *
 * There is one recognizer interface per thread/line.
 */
typedef struct VXIrecInterface {
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
   * Reset for a new speaker
   *
   * This must be called for each new caller, allowing for
   * caller-specific recognition adaptation to occur.
   *
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                          arguments for the new session
   *
   * @return VXIrec_RESULT_SUCCESS on success 
   */
  VXIrecResult (*BeginSession)(struct VXIrecInterface  *pThis,  
                               VXIMap                 *args);

  /**
   * Performs cleanup at the end of a speaker's session
   *
   * This must be called at the termination of a call, allowing for
   * call specific termination to occur. For some implementations, this 
   * can be a no-op. For others runtime resources may be released or other
   * adaptation may be completed.
   *
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                          arguments for ending the session
   *
   * @return VXIrec_RESULT_SUCCESS on success
   */
  VXIrecResult (*EndSession)(struct VXIrecInterface  *pThis, 
                             VXIMap                 *args);

  /**
   * Load a grammar from a URI, typically non-blocking.
   *
   * Initiates grammar creation from a URI (Universal Resource
   * Identifier).  The resulting grammar must be activated before use.
   * This allows for multiple grammars to be loaded when the page is
   * read.  Grammar creation may proceed either synchronously or
   * asynchronously, but must be available once ActivateGrammar( )
   * returns.
   *
   * The supported grammar formats are determined by the implementor,
   * but certain VoiceXML built-in types (e.g., grammar
   * src=builtin:grammar/digits?length=5) must be supported.
   *
   * @param properties  <b>[IN]</b>  Set of properties as defined above
   * @param type        <b>[IN]</b>  MIME type of the grammar, one of the
   *                                 REC_MIME_[...] defines above or an
   *                                 implementation defined grammar type
   * @param uri         <b>[IN]</b>  URI of the grammar definition
   * @param uriArgs     <b>[IN]</b>  Caching, timeout, base URI and other properties
   *                                 associated with URI retrieval.
   * @param gram        <b>[OUT]</b> Handle to the new grammar 
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_UNSUPPORTED_FORMAT,<br>
   *         VXIrec_RESULT_UNSUPPORTED_LANGUAGE,<br>
   *         VXIrec_RESULT_UNSUPPORTED_BUILTIN,<br>
   *         VXIrec_RESULT_NO_AUTHORIZATION i.e: restricted builtin uri,<br>
   *         VXIrec_RESULT_FETCH_TIMEOUT,<br>
   *         VXIrec_RESULT_FETCH_ERROR,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*LoadGrammarURI)(struct VXIrecInterface  *pThis,
                                 const VXIMap            *properties,
                                 const VXIchar           *type, 
                                 const VXIchar           *uri,
                                 const VXIMap            *uriArgs,
                                 VXIrecGrammar          **gram);

  /**
   * Load an inline grammar, typically non-blocking
   *
   * Initiates grammar creation from a string inlined in the VoiceXML
   * page.  As with URIs, the resulting grammar must be activated
   * before use and creation may be either synchronous or
   * asynchronous.  See LoadGrammarURI( ). <p>
   *
   * The supported grammar formats are determined by the implementor,
   * but support for a minimal JSGF fragment of the following form is
   * required: <p>
   *
   *    "vocab_item {value} | vocab_item {value} | vocab_item" <p>
   *
   * This format is used by the VoiceXML interpreter for &lt;field&gt; and
   * in some other cases. 
   *
   * @param properties  <b>[IN]</b>  Set of properties as defined above
   * @param type        <b>[IN]</b>  MIME type of the grammar, one of the
   *                                 REC_MIME_[...] defines above or an
   *                                 implementation defined grammar type
   * @param gramDef     <b>[IN]</b>  String containing the grammar definition
   * @param gram        <b>[OUT]</b> Handle to the new grammar 
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_UNSUPPORTED_FORMAT,<br>
   *         VXIrec_RESULT_UNSUPPORTED_LANGUAGE,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*LoadGrammarString)(struct VXIrecInterface  *pThis,
                                    const VXIMap            *properties,
                                    const VXIchar           *type,
                                    const VXIchar           *gramDef,
                                    VXIrecGrammar          **gram);

  /**
   * Activate a loaded VXIrecGrammar for subsequent recognition calls
   *
   * Multiple grammars may be activated before invoking Recognize( ).
   * Once this returns, the grammar must be fully loaded and ready for
   * recognition. See also LoadGrammarURI( ).
   *
   * @param properties  <b>[IN]</b>  Set of properties as defined above
   * @param gram        <b>[IN]</b>  Grammar to activate
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_UNSUPPORTED_FORMAT,<br>
   *         VXIrec_RESULT_UNSUPPORTED_LANGUAGE,<br>
   *         VXIrec_RESULT_UNSUPPORTED_BUILTIN,<br>
   *         VXIrec_RESULT_NO_AUTHORIZATION i.e: restricted builtin uri,<br>
   *         VXIrec_RESULT_FETCH_TIMEOUT,<br>
   *         VXIrec_RESULT_FETCH_ERROR,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*ActivateGrammar)(struct VXIrecInterface  *pThis,
                                  const VXIMap            *properties,
                                  VXIrecGrammar           *gram);

  /**
   * Deactivate a loaded VXIrecGrammar for subsequent recognition calls.
   *
   * Deactivate a grammar on this interface which should be used by one 
   * line.  Subsequent recognitions will not use this grammar in the 
   * recognition.
   *
   * @param gram        <b>[IN]</b>  Grammar to deactivate
   *
   * @return VXIrec_RESULT_SUCCESS on success
   */
  VXIrecResult (*DeactivateGrammar)(struct VXIrecInterface  *pThis,
                                    VXIrecGrammar           *gram);

  /**
   * Free a loaded grammar.
   * 
   * Signals that the grammar is no longer needed.  The implementation
   * can recover resources as it sees fit.
   *
   * @param gram       <b>[IN/OUT]</b> Grammar to free, the pointer is set to
   *                                   NULL
   *
   * @return VXIrec_RESULT_SUCCESS on success 
   */
  VXIrecResult (*FreeGrammar)(struct VXIrecInterface  *pThis,
                              VXIrecGrammar          **gram);

  /**
   * Recognize against the currently active grammars.
   *
   * Blocks until recognition completes (possibly unsuccessfully),
   * DTMF input completes, a timeout occurs, or an error
   * occurs. Recognition results are returned in a structure. All
   * activated grammars are deactivated after the recognition.
   *
   * @param properties   <b>[IN]</b>  Set of properties as defined above
   * @param recogResult  <b>[OUT]</b> Newly allocated result structure 
   *                                  containing the result of the
   *                                  recognize operation, see the structure
   *                                  definition above
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_MAX_SPEECH_TIMEOUT,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*Recognize)(struct VXIrecInterface    *pThis,
                            const VXIMap              *properties,
                            VXIrecRecognitionResult  **recogResult);

  /**
   * Record audio content.
   *
   * Blocks until termination input occurs, a timeout occurs, or an
   * error occurs. Record results are returned in a structure.
   *
   * @param properties   <b>[IN]</b>  Set of properties as defined above
   * @param recordResult <b>[OUT]</b> Newly allocated result structure 
   *                                  containing the result of the
   *                                  record operation, see the structure
   *                                  definition above
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_UNSUPPORTED_FORMAT,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*Record)(struct VXIrecInterface  *pThis,
                         const VXIMap            *properties,
                         VXIrecRecordResult     **recordResult);

  /**
   * GetMatchedGrammar returns grammar for ID in recognition result.
   *
   * An XML result returned by a successful recognition contains a string
   * identifying the matched grammar.  This function is use to map that 
   * string back to the corresponding VXIrecGrammar.
   *
   * @param grammarID     <b>[IN]</b> String identifier from the XML result.
   * @param gram          <b>[OUT]</b> Corresponding VXIrecGrammar.
   *
   * @return VXIrec_RESULT_SUCCESS on success
   */
  VXIrecResult (*GetMatchedGrammar)(struct VXIrecInterface * pThis,
                                    const VXIchar *grammarID,
                                    const VXIrecGrammar **gram);

  /**
   * LoadGrammarOption may build a grammar for a VXML &lt;option&gt; element.
   *
   * VoiceXML 2.0 requires support for the SRGS XML format but provides no
   * standard for building semantic results inside the grammar.  The &lt;option&gt;
   * element is unique in that it requires this mapping.  The interpreter will
   * invoke this function to build each option grammar.  For each &lt;field&gt;,
   * LoadGrammarOption may be called once for speech grammars and once for
   * DTMF grammars.
   *
   * @param properties  <b>[IN]</b> Set of properties as per LoadGrammarString
   * @param gramChoices <b>[IN]</b> The utterance value for this choice
   * @param gramValues  <b>[IN]</b> The corresponding semantic meaning
   * @param gramAcceptance  <b>[IN]</b> The corresponding accept attribute value of VXIInteger type.
   *                             The possible values are 0 for exact, 1 for approximate
   * @param isDTMF      <b>[IN]</b> Is the utterance a DTMF choice?
   * @param gram        <b>[OUT]</b> Handle to the new grammar; possibly NULL.
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_UNSUPPORTED_LANGUAGE,<br>
   *         VXIrec_RESULT_OUT_OF_MEMORY,<br>
   *         VXIrec_RESULT_NO_RESOURCE,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*LoadGrammarOption)(struct VXIrecInterface  *pThis,
                                    const VXIMap            *properties,
                                    const VXIVector         *gramChoices,
                                    const VXIVector         *gramValues,
                                    const VXIVector         *gramAcceptance,
                                    const VXIbool            isDTMF,
                                    VXIrecGrammar          **gram);

  /**
   * HotwordTransfer provides for recognition terminated transfer.
   * 
   * Called by the interpreter to perform a bridged transfer.
   *
   * If a platform returns VXIrec_RESULT_UNSUPPORTED, indicating that hotword
   * recognition is not supported during this transfer, the interpreter will
   * call VXItelInterface::TransferBridge.
   *
   * @param properties     <b>[IN]</b>  termination character, length, timeouts...<br>
   *                                    TEL_TRANSFER_TYPE = "consultation" or "bridge"<br>
   *                                    TEL_TRANSFER_DATA = aai info<br>
   *                                    TEL_MAX_CALL_TIME = VXIInteger<br>
   *                                    TEL_CONNECTTIMEOUT = VXIInteger
   * @param transferDest   <b>[IN]</b>  identifier of transfer location (e.g. a
   *                                    phone number to dial or a SIP URL)
   * @param transferResult <b>[OUT]</b> Newly allocated result structure containing
   *                                    the result of the transfer request; see structure
   *                                    definition above
   *
   * @return VXIrec_RESULT_SUCCESS on success or one of the followings:<br>
   *         VXIrec_RESULT_CONNECTION_NO_AUTHORIZATION,<br>
   *         VXIrec_RESULT_CONNECTION_BAD_DESTINATION,<br>
   *         VXIrec_RESULT_NO_ROUTE,<br>
   *         VXIrec_RESULT_CONNECTION_NO_RESOURCE,<br>
   *         VXIrec_RESULT_UNSUPPORTED_URI,<br>
   *         VXIrec_RESULT_FAILURE
   */
  VXIrecResult (*HotwordTransfer)(struct VXIrecInterface * pThis,
                                  struct VXItelInterface * tel,
                                  const VXIMap *properties,
                                  const VXIchar* transferDest,
                                  VXIrecTransferResult  ** transferResult);

  /**
   * Returns whether the platform supports Hotword Transfers.
   * @param properties     <b>[IN]</b>  Rec and Transfer properties<br>
   * @param transferDest   <b>[IN]</b>  identifier of transfer location (e.g. a
   *                                    phone number to dial or a SIP URL)
   * @return TRUE if the platform supports HotwordTransfer, otherwise FALSE.
   */
  VXIbool (*SupportsHotwordTransfer)(struct VXIrecInterface *pThis,
                                  const VXIMap  * properties,
                                  const VXIchar * transferDest);

} VXIrecInterface;

#ifdef __cplusplus
}
#endif

/*@}*/

#include "VXIheaderSuffix.h"

#endif  /* include guard */
