
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

#ifndef VXITEL_H
#define VXITEL_H

#include "VXItypes.h"                  /* For VXIchar definition */
#include "VXIvalue.h"

#include "VXIheaderPrefix.h"
#ifdef VXITEL_EXPORTS
#define VXITEL_API SYMBOL_EXPORT_DECL
#else
#define VXITEL_API SYMBOL_IMPORT_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup VXItel Telephony interface
 *
 * Abstract interface for telephony functionality.
 *
 * VXItel provides the telephony functions for the VXI.  The transfer
 * type is split into bridge and blind transfers.  These platform
 * functions are platform and generally location dependant.
 */

/*@{*/

/**
 * @name Properties
 * Keys identifying properties in a VXIMap passed to VXItelInterface::TransferBlind,
 * VXItelInterface::TransferBridge, and VXItelInterface::TransferConsultation.
 */
/*@{*/

/**
 * VXIInteger containing the connecttimeout property (in milliseconds).
 */
#define TEL_CONNECTTIMEOUT      L"vxi.tel.connecttimeout"
/**
 * VXIInteger containing the maxcalltime property (in milliseconds).
 */
#define TEL_MAX_CALL_TIME       L"vxi.tel.maxcalltime"
/**
 * VXIString containing the aai information.
 */
#define TEL_TRANSFER_DATA       L"vxi.tel.transfer.aai"
/**
 * VXIString containing the type information.
 */
#define TEL_TRANSFER_TYPE       L"vxi.tel.transfer.type"
/*@}*/

/**
 * @name Return keys
 * Keys identifying properties in a VXIMap returned by calls to
 * VXItelInterface::TransferConsultation and VXItelInterface::TransferBridge.
 */
/*@{*/

/**
 * VXIInteger containing the transfer duration in milliseconds.
 */
#define TEL_TRANSFER_DURATION   L"vxi.tel.transfer.duration"
/**
 * VXIInteger containing the status of the transfer, as defined by VXItelTransferStatus.
 */
#define TEL_TRANSFER_STATUS     L"vxi.tel.transfer.status"
/*@}*/

/**
 * Result codes for the telephony interface 
 *
 * Result codes less then zero are severe errors (likely to be
 * platform faults), those greater then zero are warnings (likely to
 * be application issues) 
 */
typedef enum VXItelResult {
  /** Fatal error, terminate call    */
  VXItel_RESULT_FATAL_ERROR        = -100,
  /** Low-level telephony library error */
  VXItel_RESULT_DRIVER_ERROR       =  -50,
  /** I/O error                      */
  VXItel_RESULT_IO_ERROR           =   -8,
  /** Out of memory                  */
  VXItel_RESULT_OUT_OF_MEMORY      =   -7,
  /** System error, out of service   */
  VXItel_RESULT_SYSTEM_ERROR       =   -6,
  /** Errors from platform services  */
  VXItel_RESULT_PLATFORM_ERROR     =   -5,
  /** Return buffer too small        */
  VXItel_RESULT_BUFFER_TOO_SMALL   =   -4,
  /** Property name is not valid    */
  VXItel_RESULT_INVALID_PROP_NAME  =   -3,
  /** Property value is not valid   */
  VXItel_RESULT_INVALID_PROP_VALUE =   -2,
  /** Invalid function argument      */
  VXItel_RESULT_INVALID_ARGUMENT   =   -1,
  /** Success                        */
  VXItel_RESULT_SUCCESS            =    0,
  /** Normal failure, nothing logged */
  VXItel_RESULT_FAILURE            =    1,
  /** Non-fatal non-specific error   */
  VXItel_RESULT_NON_FATAL_ERROR    =    2,
  /** Operation is not supported     */
  VXItel_RESULT_TIMEOUT            =    3,
  /** Call is not allowed to the destination */
  VXItel_RESULT_CONNECTION_NO_AUTHORIZATION = 4,
  /** The destination URI is malformed */
  VXItel_RESULT_CONNECTION_BAD_DESTINATION  = 5,    
  /** The platform is not able to place a call to the destination */
  VXItel_RESULT_CONNECTION_NO_ROUTE         = 6,
  /** The platform cannot allocate resources to place the call. */
  VXItel_RESULT_CONNECTION_NO_RESOURCE      = 7,
  /** The platform does not support the URI format. */
  VXItel_RESULT_UNSUPPORTED_URI             = 8,
  /** Operation is not supported     */
  VXItel_RESULT_UNSUPPORTED                 =  100
} VXItelResult;


/**
 * Telephony line status
 *
 * A line status of Active indicates that the line is currently
 * occupied. It may be in a call or in a transfer.
 */
typedef enum VXItelStatus
{
  /** In a call */
  VXItel_STATUS_ACTIVE,

  /** Not in call */
  VXItel_STATUS_INACTIVE

} VXItelStatus;


/**
 * Result codes for transfer requests
 */
typedef enum VXItelTransferStatus {
  /** The endpoint refused the call. */
  VXItel_TRANSFER_BUSY,

  /** There was no answer within the specified time. */
  VXItel_TRANSFER_NOANSWER,

  /** Some intermediate network refused the call. */
  VXItel_TRANSFER_NETWORK_BUSY,

  /** The call completed and was terminated by the caller. */
  VXItel_TRANSFER_NEAR_END_DISCONNECT,

  /** The call completed and was terminated by the callee. */
  VXItel_TRANSFER_FAR_END_DISCONNECT,

  /** The call completed and was terminated by the network. */
  VXItel_TRANSFER_NETWORK_DISCONNECT, 

  /** The call duration exceeded the value of maxtime attribute and was
     terminated by the platform. */
  VXItel_TRANSFER_MAXTIME_DISCONNECT,

  /** 
   * The call is connected.
   * This should only be used for consultation type transfers,
   * as bridges transfers don't expect it, and will set the field
   * var to "unknown".
   */
  VXItel_TRANSFER_CONNECTED,

  /**
   * The call was terminated due to the caller hanging up.
   */
  VXItel_TRANSFER_CALLER_HANGUP,

  /**
   * Asterisk channel not found
   */
  VXItel_TRANSFER_CHANNEL_UNAVAILABLE,

  /**
   * Asterisk detected answer
   */
  VXItel_TRANSFER_ANSWER,

  /** 
   * This value may be returned if the outcome of the transfer is unknown,
   * for instance if the platform does not support reporting the outcome of
   * transfer completion.  OpenVXI will treat this status as a successful
   * transfer.
   */
  VXItel_TRANSFER_UNKNOWN

} VXItelTransferStatus;


/**
 * Abstract interface for telephony functionality.
 *
 * VXItel provides the telephony functions for the VXI.  The transfer
 * type is split into the bridge and blind transfers.  These platform
 * functions are very platform and generally location dependant.
 *
 * There is one telephony interface per thread/line. 
 */
typedef struct VXItelInterface {
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
   * call start specific handling may be done on this call. 
   *
   * @param pThis <b>[IN]</b> the VXItel object for which the session
   *                          is starting.
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                    arguments for the new session
   *
   * @return VXItel_RESULT_SUCCESS on success 
   */
  VXItelResult (*BeginSession)(struct VXItelInterface  *pThis,  
                                 VXIMap                  *args);

  /**
   * Performs cleanup at the end of a call session.
   *
   * This must be called at the termination of a call, allowing for
   * call specific termination to occur. For some implementations, 
   * this can be a no-op. For others runtime resources may be released
   * or other adaptation may be completed.
   *
   * @param pThis <b>[IN]</b> the VXItel object for which the session is
   *                          ending.
   * @param args  <b>[IN]</b> Implementation defined input and output
   *                          arguments for ending the session.
   *
   * @return VXItel_RESULT_SUCCESS on success
   */
  VXItelResult (*EndSession)(struct VXItelInterface  *pThis, 
                             VXIMap                  *args);

  /**
   * Queries the status of the line.
   * 
   * Returns information about the line during an execution.  
   * Use to determine if the line is up or down.
   * 
   * @param pThis <b>[IN]</b> the VXItel object for which the status is queried.
   * @param status <b>[OUT]</b> A pointer to a pre-allocated holder for the status.
   *
   * @return VXItel_RESULT_SUCCESS if the status is queried successfully.
   *         VXItel_RESULT_INVALID_ARGUMENT if pThis is NULL or status is NULL.
   */
  VXItelResult (*GetStatus)(struct VXItelInterface * pThis,
                            VXItelStatus *status);

  /**
   * Immediately disconnects the caller on this line.
   *
   * Disconnect the line. This sends the hardware into the 
   * out-of-service state where it will no longer generate events.
   *
   * @param pThis <b>[IN]</b> the VXItel object for which the call is to be rejected.
   * @param namelist <b>[IN]</b> a map of namelist values (or NULL if namelist wasn't specified).
   *
   * @return VXItel_SUCCESS if operation succeeds
   */
  VXItelResult (*Disconnect)(struct VXItelInterface *  pThis, const VXIMap *namelist);

  /**
   * Performs a blind transfer.
   *
   * Perform a blind transfer into the network.  The implementation
   * will be platform dependant.  This call blocks for only as long
   * as it takes to initate the transfer.  It does not wait for a
   * connection result.<p>
   *
   * @param pThis <b>[IN]</b> the VXItel object for which the transfer is
   *                          to be performed.
   * @param properties <b>[IN]</b> termination character, length, timeouts...
   *                                 TEL_TRANSFER_DATA = aai info<br>
   * @param transferDestination <b>[IN]</b> identifier of transfer location (e.g. a
   *                                 phone number to dial or a SIP URL).
   * @param resp <b>[OUT]</b> Key/value containing the result.  Basic keys:<br>
   *                   TEL_TRANSFER_STATUS - The only valid status values are
   *                                         VXItel_TRANSFER_CALLER_HANGUP and
   *                                         VXItel_TRANSFER_UNKNOWN
   *
   * @return VXItel_SUCCESS if operation succeeds,<br>
   *         VXItel_RESULT_CONNECTION_NO_AUTHORIZATION,<br>
   *         VXItel_RESULT_CONNECTION_BAD_DESTINATION,<br>
   *         VXItel_RESULT_CONNECTION_NO_ROUTE,<br>
   *         VXItel_RESULT_CONNECTION_NO_RESOURCE,<br>
   *         VXItel_RESULT_UNSUPPORTED,<br>
   *         VXItel_RESULT_OUT_OF_MEMORY,<br>
   *         VXItel_RESULT_FAILURE
   */
  VXItelResult (*TransferBlind)(struct VXItelInterface *  pThis,
        const VXIMap *properties,
        const VXIchar *transferDestination,
        VXIMap **resp);

  /**
   * Performs a bridge transfer.
   *
   * Perform a bridge transfer into the network.  The implementation
   * will be platform dependant.  This call should block while the 
   * connection is active.<p>
   *
   * The platform is responsible for stopping any audio that is still
   * playing.  This would be done either when the callee answers, or,
   * if the platform allows the caller to hear the network audio, when 
   * the transfer is started.<p>
   *
   * <b>NOTE:</b> This is called only if VXIrecInterface::HotwordTransfer returns
   * VXItel_RESULT_UNSUPPORTED.
   *
   * @param pThis      <b>[IN]</b> the VXItel object for which the transfer is
   *                               to be performed.
   * @param properties <b>[IN]</b> termination character, length, timeouts...<br>
   *                                 TEL_TRANSFER_DATA = aai info<br>
   *                                 TEL_MAX_CALL_TIME = VXIInteger<br>
   *                                 TEL_CONNECTTIMEOUT = VXIInteger
   * @param transferDestination <b>[IN]</b> identifier of transfer location (e.g. a
   *                                 phone number to dial or a SIP URL).
   * @param resp <b>[OUT]</b> Key/value containing the result.  Basic keys:<br>
   *                   TEL_TRANSFER_DURATION - length of the call in ms<br>
   *                   TEL_TRANSFER_STATUS - see VXItelTransferStatus
   *
   * @return VXItel_SUCCESS if operation succeeds,<br>
   *         VXItel_RESULT_CONNECTION_NO_AUTHORIZATION,<br>
   *         VXItel_RESULT_CONNECTION_BAD_DESTINATION,<br>
   *         VXItel_RESULT_CONNECTION_NO_ROUTE,<br>
   *         VXItel_RESULT_CONNECTION_NO_RESOURCE,<br>
   *         VXItel_RESULT_UNSUPPORTED,<br>
   *         VXItel_RESULT_OUT_OF_MEMORY,<br>
   *         VXItel_RESULT_FAILURE
   */
  VXItelResult (*TransferBridge)(struct VXItelInterface *  pThis,
         const VXIMap *properties,
         const VXIchar* transferDestination,
         VXIMap **resp);

  /**
   * Performs a consultation transfer.
   *
   * Perform a consultation transfer into the network.  The implementation
   * will be platform dependant.  This call should block until the 
   * connection is completed (the callee answers, or otherwise ends in
   * no answer, busy, etc...).<p>
   *
   * The platform is responsible for stopping any audio that is still
   * playing.  This would be done either when the callee answers, or,
   * if the platform allows the caller to hear the network audio, when 
   * the transfer is started.<p>
   *
   * <b>NOTE:</b> This is called only if VXIrecInterface::HotwordTransfer returns
   * VXItel_RESULT_UNSUPPORTED.
   *
   * @param pThis      <b>[IN]</b> the VXItel object for which the transfer is
   *                               to be performed.
   * @param properties <b>[IN]</b> termination character, length, timeouts...<br>
   *                                 TEL_TRANSFER_DATA = aai info<br>
   *                                 TEL_MAX_CALL_TIME = VXIInteger<br>
   *                                 TEL_CONNECTTIMEOUT = VXIInteger
   * @param transferDestination <b>[IN]</b> identifier of transfer location (e.g. a
   *                                 phone number to dial or a SIP URL).
   * @param resp <b>[OUT]</b> Key/value containing the result.  Basic keys:<br>
   *                          TEL_TRANSFER_DURATION - length of the call in ms<br>
   *                          TEL_TRANSFER_STATUS - see VXItelTransferStatus
   *
   * @return VXItel_SUCCESS if operation succeeds,<br>
   *         VXItel_RESULT_CONNECTION_NO_AUTHORIZATION,<br>
   *         VXItel_RESULT_CONNECTION_BAD_DESTINATION,<br>
   *         VXItel_RESULT_CONNECTION_NO_ROUTE,<br>
   *         VXItel_RESULT_CONNECTION_NO_RESOURCE,<br>
   *         VXItel_RESULT_UNSUPPORTED,<br>
   *         VXItel_RESULT_OUT_OF_MEMORY,<br>
   *         VXItel_RESULT_FAILURE
   */
  VXItelResult (*TransferConsultation)(struct VXItelInterface *  pThis,
         const VXIMap *properties,
         const VXIchar* transferDestination,
         VXIMap **resp);
} VXItelInterface;
/*@}*/

#ifdef __cplusplus
}
#endif

#include "VXIheaderSuffix.h"

#endif  /* include guard */
