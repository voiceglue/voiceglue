
/****************License************************************************
 * Vocalocity OpenVXI
 * Copyright (C) 2004-2005 by Vocalocity, Inc. All Rights Reserved.
 * vglue mods Copyright 2006,2009 Ampersand Inc., Doug Campbell
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

#include "SWIutfconversions.h"
#include <VXItrd.h>

#include <cstdio>
#include <string>
#include <cstring>
#include <sstream>
#define VXIstrcmp wcscmp
#include <syslog.h>

#include <vglue_tostring.h>
#include <vglue_ipc.h>
#include <vglue_prompt.h>

#include <VXIvalue.h>
#define VXIPROMPT_EXPORTS
#include "VXIpromptAPI.h"

// Global for the base diagnostic tag ID
//
static VXIunsigned gblDiagLogBase = 0;

// Constants for diagnostic logging tags
//
static const VXIunsigned DIAG_TAG_PROMPTING = 1;
static const VXIunsigned DIAG_TAG_PREFETCHING = 2;


// VXIprompt implementation of the VXIprompt interface
//
extern "C" {
struct VXIpromptImpl {
  // Base interface, must be first
  VXIpromptInterfaceEx intf;

  // Log interface for this resource
  VXIlogInterface *log;

  // Internet fetch interface for this resource
  VXIinetInterface *inet;
};
}


// A few conversion functions...

static inline VXIpromptImpl * ToVXIpromptImpl(VXIpromptInterface * i)
{ return reinterpret_cast<VXIpromptImpl *>(i); }

static inline VXIpromptImpl * ToVXIpromptImpl(VXIpromptInterfaceEx * i)
{ return reinterpret_cast<VXIpromptImpl *>(i); }


/*******************************************************
 *
 * Utility functions
 *
 *******************************************************/ 

/**
 * Log an error
 */
static VXIlogResult Error(VXIpromptImpl *impl, VXIunsigned errorID,
			  const VXIchar *format, ...)
{
  VXIlogResult rc;
  va_list arguments;

  if ((! impl) || (! impl->log))
    return VXIlog_RESULT_NON_FATAL_ERROR;
  
  if (format) {
    va_start(arguments, format);
    rc = (*impl->log->VError)(impl->log, COMPANY_DOMAIN L".VXIprompt", 
			      errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*impl->log->Error)(impl->log, COMPANY_DOMAIN L".VXIprompt",
			     errorID, NULL);
  }

  return rc;
}


/**
 * Log a diagnostic message
 */
static VXIlogResult Diag(VXIpromptImpl *impl, VXIunsigned tag, 
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
 * Method routines for VXIpromptInterface structure
 *
 *******************************************************/ 

// Get the VXIprompt interface version supported
//
static VXIint32 VXIpromptGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}


// Get the implementation name
//
static const VXIchar* VXIpromptGetImplementationName(void)
{
  static const VXIchar IMPLEMENTATION_NAME[] = COMPANY_DOMAIN L".VXIprompt";
  return IMPLEMENTATION_NAME;
}


// Begin a session
//
static 
VXIpromptResult VXIpromptBeginSession(VXIpromptInterface * pThis, VXIMap *)
{
  return VXIprompt_RESULT_SUCCESS;
}


// End a session
//
static
VXIpromptResult VXIpromptEndSession(VXIpromptInterface *pThis, VXIMap *)
{
  return VXIprompt_RESULT_SUCCESS;
}


// Start playing queued prompts. This call is non-blocking.
//
static VXIpromptResult VXIpromptPlay(VXIpromptInterface * vxip)
{
  VXIpromptImpl *impl = ToVXIpromptImpl(vxip);

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      voiceglue_log ((char) LOG_DEBUG, "VXIpromptPlay()");
  };

  voiceglue_sendipcmsg ("Play\n");
  std::string ipcmsg_result = voiceglue_getipcmsg();
  if (ipcmsg_result.compare ("Played") != 0)
  {
      if (voiceglue_loglevel() >= LOG_ERR)
      {
	  std::ostringstream logstring;
	  logstring << "Play got invalid response: "
		    << ipcmsg_result;
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };
      return VXIprompt_RESULT_FAILURE;
  };

  Diag(impl, DIAG_TAG_PROMPTING, NULL, L"Playing queued prompts");
  return VXIprompt_RESULT_SUCCESS;
}


// Start the special play of a filler prompt. This call is non-blocking.
//
static
VXIpromptResult VXIpromptPlayFiller(VXIpromptInterface * vxip,
				    const VXIchar *type,
				    const VXIchar *src,
				    const VXIchar *text,
				    const VXIMap* properties,
				    VXIlong minPlayMsec)
{
  return VXIprompt_RESULT_SUCCESS;
}


static
VXIpromptResult VXIpromptPrefetch(VXIpromptInterface * vxip,
				  const VXIchar *type,
				  const VXIchar *src,
				  const VXIchar *text,
				  const VXIMap* properties)
{
  VXIpromptImpl *impl = ToVXIpromptImpl(vxip);

  Diag(impl, DIAG_TAG_PREFETCHING, L"VXIpromptPrefetch", L"%s",
      (text ? text : L"NULL")); 

  return VXIprompt_RESULT_SUCCESS;
}


// Queue prompt for playing. This call is non-blocking. The prompt
//  is not played until VXIpromptPlay( ) is called.
// 
static
VXIpromptResult VXIpromptQueue(VXIpromptInterface* vxip,
			       const VXIchar *raw_type,
			       const VXIchar *raw_src,  /* no longer used */
			       const VXIchar *raw_text,
			       const VXIMap  *properties)
{
  VXIpromptImpl *impl = ToVXIpromptImpl(vxip);
  
  vxistring type(L""), text(L"");
  if (raw_type)
    type = raw_type;
  if (raw_text)
    text = raw_text;

  if (text.empty())
    return VXIprompt_RESULT_INVALID_ARGUMENT;

  // currently, vxi only queues SSML
  if (type == VXI_MIME_SSML) {
    vxistring bargein;
    const VXIValue *val = VXIMapGetProperty(properties, L"bargein");
	if (val != NULL)
      bargein = VXIStringCStr(reinterpret_cast<const VXIString*>(val));

    vxistring bargeintype;
    val = VXIMapGetProperty(properties, L"bargeintype");
    if (val != NULL)
	bargeintype = VXIStringCStr(reinterpret_cast<const VXIString*>(val));
    Diag(impl, DIAG_TAG_PROMPTING, NULL, L"Queuing TTS: bargein=%s, bargeintype=%s, ssml=%s",
         bargein.c_str(), bargeintype.c_str(), text.c_str());

    VXIpromptResult vg_result =
	voiceglue_prompt (raw_text, properties);

    return vg_result;
  } 
  else {
    Diag(impl, DIAG_TAG_PROMPTING, NULL, 
         L"Queuing Unknown type text (%s): %s" , type.c_str(), text.c_str());
    return VXIprompt_RESULT_UNSUPPORTED;
  }

  return VXIprompt_RESULT_SUCCESS;
}

             
// Wait until all queued prompts finish playing.
//
static VXIpromptResult VXIpromptWait(VXIpromptInterface* vxip,
                                     VXIpromptResult* playResult)
{
  VXIpromptImpl *impl = ToVXIpromptImpl(vxip);

  if (voiceglue_loglevel() >= LOG_DEBUG)
  {
      voiceglue_log ((char) LOG_DEBUG, "VXIpromptWait()");
  };

  voiceglue_sendipcmsg ("Wait\n");
  std::string wait_response = voiceglue_getipcmsg();

  Diag(impl, DIAG_TAG_PROMPTING, NULL, 
       L"%s" , L"VXIpromptWait");
  *playResult = VXIprompt_RESULT_SUCCESS;
  return VXIprompt_RESULT_SUCCESS;
}

// Stop the current playing prompt.
//
static VXIpromptResult VXIpromptStop (VXIpromptInterfaceEx *prompt)
{
  if (prompt == NULL)
    return VXIprompt_RESULT_INVALID_ARGUMENT;
  VXIpromptImpl* promptImpl = reinterpret_cast<VXIpromptImpl*>(prompt);
  return VXIprompt_RESULT_SUCCESS;  
}

/*******************************************************
 * Global init and factory methods
 *******************************************************/ 

VXIPROMPT_API VXIpromptResult VXIpromptInit (VXIlogInterface  *log,
					     VXIunsigned       diagLogBase,
					     const VXIVector   *resources,
					     VXIMap            *args)
{ 
  if (! log) return VXIprompt_RESULT_INVALID_ARGUMENT;

  gblDiagLogBase = diagLogBase;
  return VXIprompt_RESULT_SUCCESS; 
}


VXIPROMPT_API VXIpromptResult VXIpromptShutDown (VXIlogInterface  *log)
{ 
  if (! log) return VXIprompt_RESULT_INVALID_ARGUMENT;

  return VXIprompt_RESULT_SUCCESS; 
}

VXIPROMPT_API
VXIpromptResult VXIpromptCreateResource (
  VXIlogInterface     *log,
  VXIinetInterface    *inet,
  VXIcacheInterface   *cache,
  VXIpromptInterface **prompt)
{  
  if ((! log) || (! inet)) return VXIprompt_RESULT_INVALID_ARGUMENT;

  VXIpromptImpl* pp = new VXIpromptImpl();
  if (pp == NULL) return VXIprompt_RESULT_OUT_OF_MEMORY;

  pp->log = log;
  pp->inet = inet;
  pp->intf.vxiprompt.GetVersion            = VXIpromptGetVersion;
  pp->intf.vxiprompt.GetImplementationName = VXIpromptGetImplementationName;
  pp->intf.vxiprompt.BeginSession          = VXIpromptBeginSession;
  pp->intf.vxiprompt.EndSession            = VXIpromptEndSession;
  pp->intf.vxiprompt.Play                  = VXIpromptPlay;
  pp->intf.vxiprompt.PlayFiller            = VXIpromptPlayFiller;
  pp->intf.vxiprompt.Prefetch              = VXIpromptPrefetch;
  pp->intf.vxiprompt.Queue                 = VXIpromptQueue;
  pp->intf.vxiprompt.Wait                  = VXIpromptWait;
  pp->intf.Stop                            = VXIpromptStop;
  
  *prompt = &pp->intf.vxiprompt;
  return VXIprompt_RESULT_SUCCESS;
}

VXIPROMPT_API 
VXIpromptResult VXIpromptDestroyResource (VXIpromptInterface **prompt)
{
  if (prompt == NULL || *prompt == NULL)
    return VXIprompt_RESULT_INVALID_ARGUMENT;
  VXIpromptImpl* promptImpl = reinterpret_cast<VXIpromptImpl*>(*prompt);

  delete promptImpl;
  *prompt = NULL;

  return VXIprompt_RESULT_SUCCESS;
}
