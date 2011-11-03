
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

#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <cstring>
#define VXIstrcmp wcscmp
#include <syslog.h>

#include <vglue_tostring.h>
#include <vglue_ipc.h>
#include <vglue_tel.h>

#include <VXIvalue.h>
#include "SWIutfconversions.h"
#include <VXItrd.h>
#define VXITEL_EXPORTS
#include "VXItelAPI.h"
#include "SWIstring.h"
typedef std::basic_string<VXIchar> vxistring;
  
// ------*---------*---------*---------*---------*---------*---------*---------


// Global for the base diagnostic tag ID
//
static VXIunsigned gblDiagLogBase = 0;

// Constants for diagnostic logging tags
//
static const VXIunsigned DIAG_TAG_SIGNALING = 0;

static inline VXItelTransferStatus GetXferStatus( const wchar_t* hupStrId )
{
  if( hupStrId )
  {
    if( wcscmp(hupStrId, L"far_end_disconnect") == 0 )
      return VXItel_TRANSFER_FAR_END_DISCONNECT;
    else if( wcscmp(hupStrId, L"near_end_disconnect") == 0 )
      return VXItel_TRANSFER_NEAR_END_DISCONNECT;
    else if( wcscmp(hupStrId, L"busy") == 0 )
      return VXItel_TRANSFER_BUSY;
    else if( wcscmp(hupStrId, L"noanswer") == 0 )
      return VXItel_TRANSFER_NOANSWER;
    else if( wcscmp(hupStrId, L"network_busy") == 0 )
      return VXItel_TRANSFER_NETWORK_BUSY;
    else if( wcscmp(hupStrId, L"network_disconnect") == 0 )
      return VXItel_TRANSFER_NETWORK_DISCONNECT;
    else if( wcscmp(hupStrId, L"maxtime_disconnect") == 0 )
      return VXItel_TRANSFER_MAXTIME_DISCONNECT;
    else if( wcscmp(hupStrId, L"caller_hangup") == 0 )
      return VXItel_TRANSFER_CALLER_HANGUP;
    else if( wcscmp(hupStrId, L"ANSWER") == 0 )
      return VXItel_TRANSFER_ANSWER;
    else if( wcscmp(hupStrId, L"BUSY") == 0 )
      return VXItel_TRANSFER_BUSY;
    else if( wcscmp(hupStrId, L"CHANUNAVAIL") == 0 )
      return VXItel_TRANSFER_CHANNEL_UNAVAILABLE;
  }
  return VXItel_TRANSFER_UNKNOWN; 
}

static inline VXItelTransferStatus GetXferStatusAsterisk(std::string hupStrId )
{
  if( !hupStrId.empty() )
  {
    if( hupStrId.compare("far_end_disconnect") == 0 )
      return VXItel_TRANSFER_FAR_END_DISCONNECT;
    else if( hupStrId.compare("near_end_disconnect") == 0 )
      return VXItel_TRANSFER_NEAR_END_DISCONNECT;
    else if( hupStrId.compare("busy") == 0 )
      return VXItel_TRANSFER_BUSY;
    else if( hupStrId.compare("noanswer") == 0 )
      return VXItel_TRANSFER_NOANSWER;
    else if( hupStrId.compare( "network_busy") == 0 )
      return VXItel_TRANSFER_NETWORK_BUSY;
    else if( hupStrId.compare( "network_disconnect") == 0 )
      return VXItel_TRANSFER_NETWORK_DISCONNECT;
    else if( hupStrId.compare("maxtime_disconnect") == 0 )
      return VXItel_TRANSFER_MAXTIME_DISCONNECT;
    else if( hupStrId.compare( "caller_hangup") == 0 )
      return VXItel_TRANSFER_CALLER_HANGUP;
	else if( hupStrId.compare( "ANSWER") == 0 )
      return VXItel_TRANSFER_ANSWER;
	else if( hupStrId.compare( "BUSY") == 0 )
      return VXItel_TRANSFER_BUSY;
	else if( hupStrId.compare("NOANSWER") == 0 )
      return VXItel_TRANSFER_NOANSWER;
	else if( hupStrId.compare( "CHANUNAVAIL") == 0 )
      return VXItel_TRANSFER_CHANNEL_UNAVAILABLE;
	  
  }
  return VXItel_TRANSFER_UNKNOWN; 
}

// VXItel implementation of the VXItel interface
//
class VXItelImpl {
public:
  VXItelImpl()
  : statusMutex(NULL), status(VXItel_STATUS_INACTIVE)
  {
    VXItrdMutexCreate(&statusMutex);
  }  
  ~VXItelImpl()
  {
    VXItrdMutexDestroy(&statusMutex);
  }
  
  // Set Line Status
  void SetLineStatus(VXItelStatus st)
  {
    VXItrdMutexLock(statusMutex);
    status = st;
    VXItrdMutexUnlock(statusMutex);
  }

  // Get Line Status
  VXItelStatus GetLineStatus(void)
  {
      VXItelStatus stat = voiceglue_get_line_status();
      SetLineStatus (stat);
      return stat;
  }
  
  // Base interface, must be first
  VXItelInterfaceEx telIntf;

  // Log interface for this resource
  VXIlogInterface *log;
  
  // Line status
  VXItelStatus status;
  
  // Mutex for line status
  VXItrdMutex *statusMutex;
};


// A few conversion functions...

static inline VXItelImpl * ToVXItelImpl(VXItelInterface * i)
{ return reinterpret_cast<VXItelImpl *>(i); }

static inline VXItelImpl * ToVXItelImpl(VXItelInterfaceEx * i)
{ return reinterpret_cast<VXItelImpl *>(i); }


/*******************************************************
 *
 * Utility functions
 *
 *******************************************************/ 

/**
 * Log an error
 */
static VXIlogResult Error(VXItelImpl *impl, VXIunsigned errorID,
                          const VXIchar *format, ...)
{
  VXIlogResult rc;
  va_list arguments;

  if ((! impl) || (! impl->log))
    return VXIlog_RESULT_NON_FATAL_ERROR;
  
  if (format) {
    va_start(arguments, format);
    rc = (*impl->log->VError)(impl->log, COMPANY_DOMAIN L".VXItel", 
                              errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*impl->log->Error)(impl->log, COMPANY_DOMAIN L".VXItel",
                             errorID, NULL);
  }

  return rc;
}


/**
 * Log a diagnostic message
 */
static VXIlogResult Diag(VXItelImpl *impl, VXIunsigned tag, 
                         const VXIchar *subtag, const VXIchar *format, ...)
{
  VXIlogResult rc;
  va_list arguments;

  if ((! impl) || (! impl->log))
    return VXIlog_RESULT_NON_FATAL_ERROR;

  if (format) {
    va_start(arguments, format);
    rc = (*impl->log->VDiagnostic)(impl->log, tag + gblDiagLogBase, subtag,
                                   format, arguments);
    va_end(arguments);
  } else {
    rc = (*impl->log->Diagnostic)(impl->log, tag + gblDiagLogBase, subtag,
                                  NULL);
  }

  return rc;
}


/*******************************************************
 *
 * Method routines for VXItelInterface structure
 *
 *******************************************************/ 

// Get the VXItel interface version supported
//
static VXIint32 VXItelGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}



// Get the implementation name
//
static const VXIchar* VXItelGetImplementationName(void)
{
  static const VXIchar IMPLEMENTATION_NAME[] = COMPANY_DOMAIN L".VXItel";
  return IMPLEMENTATION_NAME;
}


// Begin a session
//
static 
VXItelResult VXItelBeginSession(VXItelInterface * pThis, VXIMap *)
{
  if( !pThis ) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl *impl = ToVXItelImpl(pThis);
  impl->SetLineStatus(VXItel_STATUS_ACTIVE);
  return VXItel_RESULT_SUCCESS;
}


// End a session
//
static
VXItelResult VXItelEndSession(VXItelInterface *pThis, VXIMap *)
{
  if( !pThis ) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl *impl = ToVXItelImpl(pThis);
  impl->SetLineStatus(VXItel_STATUS_INACTIVE);
  return VXItel_RESULT_SUCCESS;
}


static
VXItelResult VXItelGetStatus(VXItelInterface * pThis, VXItelStatus *status)
{
  if( !pThis ) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl *impl = ToVXItelImpl(pThis);
  *status = impl->GetLineStatus();
  return VXItel_RESULT_SUCCESS;
}

/**
 * Disconnect caller (ie hang up).
 *
 * @param  ch   A handle to the channel resource manager
 */
static
VXItelResult VXItelDisconnect(VXItelInterface * pThis,const VXIMap *namelist)
{
  if( !pThis ) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl *impl = ToVXItelImpl(pThis);
  Diag(impl, DIAG_TAG_SIGNALING, NULL, L"Disconnect");
  impl->SetLineStatus(VXItel_STATUS_INACTIVE); 
  voiceglue_sendipcmsg ("Disconnect\n");
  return VXItel_RESULT_SUCCESS;
}


static bool GetReturnCode(const VXIMap* props, VXItelResult& retcode)
{
  const VXIValue* val = VXIMapGetProperty(props, L"ReturnCode");
  if (val != NULL && VXIValueGetType(val) == VALUE_STRING) {
    const VXIchar* strval = VXIStringCStr(reinterpret_cast<const VXIString*>(val));
    if( strval ) {
      if( wcscmp(strval, L"VXItel_RESULT_CONNECTION_NO_AUTHORIZATION") == 0 ) {
        retcode = VXItel_RESULT_CONNECTION_NO_AUTHORIZATION;
        return true;  
      }        
      else if( wcscmp(strval, L"VXItel_RESULT_CONNECTION_BAD_DESTINATION") == 0 ) {
        retcode = VXItel_RESULT_CONNECTION_BAD_DESTINATION;
        return true;  
      }        
      else if( wcscmp(strval, L"VXItel_RESULT_CONNECTION_NO_ROUTE") == 0 ) {
        retcode = VXItel_RESULT_CONNECTION_NO_ROUTE;
        return true;  
      }        
      else if( wcscmp(strval, L"VXItel_RESULT_CONNECTION_NO_RESOURCE") == 0 ) {
        retcode = VXItel_RESULT_CONNECTION_NO_RESOURCE;
        return true;  
      }        
      else if( wcscmp(strval, L"VXItel_RESULT_UNSUPPORTED_URI") == 0 ) {
        retcode = VXItel_RESULT_UNSUPPORTED_URI;
        return true;  
      }        
      else if( wcscmp(strval, L"VXItel_RESULT_UNSUPPORTED") == 0 ) {
        retcode = VXItel_RESULT_UNSUPPORTED;
        return true;  
      }        
    }   
  }
  return false;
}

/**
 * Blind Transfer.
 */
static
VXItelResult VXItelTransferBlind(VXItelInterface * vxip, 
                                 const VXIMap * prop,
                                 const VXIchar * transferDestination,
                                 VXIMap **resp)
{
  VXItelImpl *impl = ToVXItelImpl(vxip);
  Diag(impl, DIAG_TAG_SIGNALING, NULL, L"TransferBlind: %s", 
       transferDestination);

  //  voiceglue code
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXItelTransferBlind called with prop: "
		<< VXIValue_to_Std_String((const VXIValue *) prop);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  std::string dest = VXIchar_to_Std_String (transferDestination);
  std::string from = VXIMap_Property_to_Std_String(prop,"vxi.tel.transfer.aai");
  std::string timeout =
      VXIMap_Property_to_Std_String(prop,"vxi.tel.connecttimeout");
  std::string result;
  voiceglue_transfer (dest, from, 0, timeout, result);

  //  original openvxi code

  *resp = VXIMapCreate();

  VXItelTransferStatus xferStatus = GetXferStatusAsterisk(result);

  const VXIValue* dval = VXIMapGetProperty(prop, L"TransferStatus");
  if( dval && VXIValueGetType(dval) == VALUE_STRING ){
    const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
    xferStatus = GetXferStatus(hid);
  }

  VXIMapSetProperty(*resp, TEL_TRANSFER_STATUS, 
                    (VXIValue *) VXIIntegerCreate(xferStatus));

  return VXItel_RESULT_SUCCESS;
}

/**
 * Consultation Transfer.
 */
static
VXItelResult VXItelTransferConsultation(VXItelInterface * vxip, 
                                 const VXIMap * prop,
                                 const VXIchar * transferDestination,
                                 VXIMap **resp)
{
  VXItelImpl *impl = ToVXItelImpl(vxip);
  Diag(impl, DIAG_TAG_SIGNALING, NULL, L"TransferConsultation: %s", 
       transferDestination);

  *resp = VXIMapCreate();

  VXItelTransferStatus xferStatus = VXItel_TRANSFER_CONNECTED;

  const VXIValue* dval = VXIMapGetProperty(prop, L"AnswerDelay");
  if( dval && VXIValueGetType(dval) == VALUE_STRING ){
    const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
    int answerDelay = SWIwtoi(hid);

    const VXIValue* ct = VXIMapGetProperty(prop, TEL_CONNECTTIMEOUT);
    if (answerDelay > VXIIntegerValue(reinterpret_cast<const VXIInteger*>(ct)))
      xferStatus = VXItel_TRANSFER_NOANSWER;
  }
  else {
    dval = VXIMapGetProperty(prop, L"TransferStatus");
    if( dval && VXIValueGetType(dval) == VALUE_STRING ){
      const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
      xferStatus = GetXferStatus(hid);
      // ignore maxtime_disconnect since it can't happen on consultation
      if (xferStatus == VXItel_TRANSFER_MAXTIME_DISCONNECT)
         xferStatus = VXItel_TRANSFER_CONNECTED;
    }
  }

  VXIMapSetProperty(*resp, TEL_TRANSFER_STATUS, 
                    (VXIValue *) VXIIntegerCreate(xferStatus));

  return VXItel_RESULT_SUCCESS;
}

/**
 * Bridging Transfer.
 *
 */
static
VXItelResult VXItelTransferBridge(VXItelInterface * vxip, 
                                  const VXIMap * prop,
                                  const VXIchar * transferDestination,
                                  VXIMap **resp)
{
  VXItelImpl *impl = ToVXItelImpl(vxip);
  Diag(impl, DIAG_TAG_SIGNALING, NULL, L"TransferBridge: %s", 
       transferDestination);

  //  voiceglue code
  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      std::ostringstream logstring;
      logstring << "VXItelTransferBridge called with prop: "
		<< VXIValue_to_Std_String((const VXIValue *) prop);
      voiceglue_log ((char) LOG_DEBUG, logstring);
  };

  std::string dest = VXIchar_to_Std_String (transferDestination);
  std::string from = VXIMap_Property_to_Std_String(prop,"vxi.tel.transfer.aai");
  std::string timeout =
      VXIMap_Property_to_Std_String(prop,"vxi.tel.connecttimeout");
  std::string result;
  voiceglue_transfer (dest, from, 1, timeout, result);

  //  original openvxi code

  *resp = VXIMapCreate();

  // check for return code
  VXItelResult rc = VXItel_RESULT_SUCCESS;
  if( GetReturnCode(prop, rc) ) return rc;

  VXItelTransferStatus xferStatus = GetXferStatusAsterisk(result);
  
  const VXIValue* dval = VXIMapGetProperty(prop, L"AnswerDelay");
  if( dval && VXIValueGetType(dval) == VALUE_STRING ){
    const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
	int answerDelay = SWIwtoi(hid);

	const VXIValue* ct = VXIMapGetProperty(prop, TEL_CONNECTTIMEOUT);
	if (answerDelay > VXIIntegerValue(reinterpret_cast<const VXIInteger*>(ct)))
	  xferStatus = VXItel_TRANSFER_NOANSWER;
  }
  else {
    dval = VXIMapGetProperty(prop, L"TransferStatus");
    if( dval && VXIValueGetType(dval) == VALUE_STRING ){
      const wchar_t* hid = VXIStringCStr(reinterpret_cast<const VXIString *>(dval));
      xferStatus = GetXferStatus(hid);
    }
  }

  VXIMapSetProperty(*resp, TEL_TRANSFER_STATUS, 
                    (VXIValue *) VXIIntegerCreate(xferStatus));
  VXIMapSetProperty(*resp, TEL_TRANSFER_DURATION, (VXIValue *) VXIIntegerCreate(12000));
  return VXItel_RESULT_SUCCESS;
}


/**
 * Enable calls on the channel
 *
 */
static VXItelResult
VXItelEnableCall(struct VXItelInterfaceEx  *pThis)
{
  if (! pThis) return VXItel_RESULT_INVALID_ARGUMENT;

  return VXItel_RESULT_SUCCESS;
}


/**
 * Wait for and answer a call on the channel
 *
 */
static VXItelResult
VXItelWaitForCall(struct VXItelInterfaceEx  *vxip,
                  VXIMap                 **telephonyProps)
{
  if ((! vxip) || (! telephonyProps))
    return VXItel_RESULT_INVALID_ARGUMENT;

  VXItelImpl *impl = ToVXItelImpl(vxip);
  Diag(impl, DIAG_TAG_SIGNALING, NULL, L"New call");

  *telephonyProps = VXIMapCreate();
  if (! *telephonyProps) return VXItel_RESULT_OUT_OF_MEMORY;

  VXIMapSetProperty(*telephonyProps, L"dnis",
                    (VXIValue *)VXIStringCreate(L"6174284444"));
  VXIMapSetProperty(*telephonyProps, L"ani", 
                    (VXIValue *)VXIStringCreate(L"6508470000"));

  return VXItel_RESULT_SUCCESS;
}


/*******************************************************
 * Factory and init routines
 *******************************************************/ 

/**
 * Global initialization of Telephony platform.
 */
VXITEL_API VXItelResult VXItelInit (VXIlogInterface  *log,
                                    VXIunsigned       diagLogBase,
                                    VXIMap            *args)
{
  if (! log) return VXItel_RESULT_INVALID_ARGUMENT;

  gblDiagLogBase = diagLogBase;
  return VXItel_RESULT_SUCCESS;
}


/**
 * Global shutdown of Telephony platform.
 */
VXITEL_API VXItelResult VXItelShutDown (VXIlogInterface  *log)
{
  if (! log) return VXItel_RESULT_INVALID_ARGUMENT;

  return VXItel_RESULT_SUCCESS;
}


/**
 * Creates an VXItel implementation of the VXItel interface
 */
VXITEL_API VXItelResult VXItelCreateResource(VXIlogInterface *log,
                                             VXItelInterface **tel)
{
  if (! log) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl* pp = new VXItelImpl();
  if (pp == NULL) return VXItel_RESULT_OUT_OF_MEMORY;

  pp->log = log;
  pp->telIntf.vxitel.GetVersion     = VXItelGetVersion;
  pp->telIntf.vxitel.GetImplementationName = VXItelGetImplementationName;
  pp->telIntf.vxitel.BeginSession   = VXItelBeginSession;
  pp->telIntf.vxitel.EndSession     = VXItelEndSession;
  pp->telIntf.vxitel.GetStatus      = VXItelGetStatus;
  pp->telIntf.vxitel.Disconnect     = VXItelDisconnect;
  pp->telIntf.vxitel.TransferBlind  = VXItelTransferBlind;
  pp->telIntf.vxitel.TransferBridge = VXItelTransferBridge;
  pp->telIntf.vxitel.TransferConsultation = VXItelTransferConsultation;
  pp->telIntf.EnableCall          = VXItelEnableCall;
  pp->telIntf.WaitForCall         = VXItelWaitForCall;

  *tel = &pp->telIntf.vxitel;
  return VXItel_RESULT_SUCCESS;
}

/**
 * Destroys the specified VXItel implementation
 */
VXITEL_API VXItelResult VXItelDestroyResource(VXItelInterface **tel)
{
  if (tel == NULL || *tel == NULL) return VXItel_RESULT_INVALID_ARGUMENT;
  VXItelImpl* telImpl = reinterpret_cast<VXItelImpl*>(*tel);

  delete telImpl;
  *tel = NULL;

  return VXItel_RESULT_SUCCESS;
}
