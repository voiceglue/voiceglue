
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

/***********************************************************************
 *
 * Top level API implementation and VXI class methods
 *
 ***********************************************************************/

#define VXI_EXPORTS
#include "VXI.hpp"
#include "VXIinterpreter.h"
#include "SimpleLogger.hpp"
#include "DocumentParser.hpp"
#include "VXIjsi.h"
#include "VXIcache.h"                 // for version check on interface

#include <syslog.h>
#include <vglue_tostring.h>
#include <vglue_ipc.h>
#include <vglue_inet.h>

/******************************************************************
 * Init / ShutDown
 ******************************************************************/

static unsigned int DEFAULT_DOCUMENT_CACHE_SIZE = 1024; // 1024 kB = 1 MB

// VXIinterp_RESULT_SUCCESS
// VXIinterp_RESULT_FAILURE
VXI_INTERPRETER 
VXIinterpreterResult VXIinterpreterInit(VXIlogInterface * log,
					VXIunsigned       diagLogBase,
                                        const VXIMap    * props)
{
  SimpleLogger::SetMessageBase(diagLogBase);

  unsigned int cacheSize = DEFAULT_DOCUMENT_CACHE_SIZE;
  if (props != NULL) {
    const VXIValue * v = VXIMapGetProperty(props, VXI_DOC_MEMORY_CACHE);
    if (v != NULL && VXIValueGetType(v) == VALUE_INTEGER)
      cacheSize = VXIIntegerValue(reinterpret_cast<const VXIInteger *>(v));
  }

  if (!DocumentParser::Initialize(cacheSize))
    return VXIinterp_RESULT_FAILURE;

  return VXIinterp_RESULT_SUCCESS;
}


VXI_INTERPRETER void VXIinterpreterShutDown(VXIlogInterface  *log)
{
  DocumentParser::Deinitialize();
}


/******************************************************************
 * Interface definition
 ******************************************************************/

// This is an interesting and legal hack.  The alignment of a structure in C
// must follow that of the first member.  The result is that the Interpreter
// structure has the same alignment as VXIinterpreterInterface.  The
// implementation may be safely appended onto the end.

extern "C" {
typedef struct Interpreter {
  VXIinterpreterInterface interface;
  SimpleLogger * logger;
  VXIcacheInterface  * cache;
  VXIinetInterface * inet;
  VXIjsiInterface * jsi;
  VXIobjectInterface * object;
  VXIpromptInterface * prompt;
  VXIrecInterface * rec;
  VXItelInterface * tel;
  VXI * implementation;
} Interpreter;
}


extern "C" VXIint32 VXIinterpreterGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}


extern "C" const VXIchar* VXIinterpreterGetImplementationName(void)
{
#ifndef COMPANY_DOMAIN
#define COMPANY_DOMAIN L"com.yourcompany"
#endif
  static const VXIchar IMPLEMENTATION_NAME[] = 
    COMPANY_DOMAIN L".VXI";
  return IMPLEMENTATION_NAME;
}


extern "C" VXIinterpreterResult VXIinterpreterRun(VXIinterpreterInterface *x,
						  const VXIchar *name,
						  const VXIchar *sessionArgs,
						  VXIValue **result)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;
  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  switch(interpreter->implementation->Run(name, sessionArgs,
                                          interpreter->logger,
                                          interpreter->inet,
                                          interpreter->cache,
                                          interpreter->jsi,
                                          interpreter->rec,
                                          interpreter->prompt,
                                          interpreter->tel,
                                          interpreter->object,
                                          result))
  {
  case -2: // Fatal error
    return VXIinterp_RESULT_FATAL_ERROR;
  case -1: // Out of memory
    return VXIinterp_RESULT_OUT_OF_MEMORY;
  case  0: // Success
    return VXIinterp_RESULT_SUCCESS;
  case  1: // Infinite loop suspected.
  case  2: // ECMAScript error
    return VXIinterp_RESULT_FAILURE;
  case  3: // Invalid startup documents
    return VXIinterp_RESULT_INVALID_DOCUMENT;
  case  4:  // Stopped
    return VXIinterp_RESULT_STOPPED;
  default:
    return VXIinterp_RESULT_FATAL_ERROR;
  }
}


extern "C" VXIinterpreterResult VXIinterpreterSetProperties(
                                                 VXIinterpreterInterface * x,
                                                 const VXIMap * props)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;
  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  // Handle the degenerate case.
  if (VXIMapNumProperties(props) == 0) return VXIinterp_RESULT_SUCCESS;

  // Walk through each property in the map.
  VXI * vxi = interpreter->implementation;
  bool badValue = false;
  bool badName = false;
  const VXIchar  * key;
  const VXIValue * value;
  VXIMapIterator * i = VXIMapGetFirstProperty(props, &key, &value);
  do {
    if (key == NULL) badName = true;
    else if (value == NULL) badValue = true;
	else {
      vxistring keyString(key);

      if (keyString == VXI_BEEP_AUDIO)
	    badValue |= !(vxi->SetRuntimeProperty(VXI::BeepURI, value));
      else if (keyString == VXI_PLATFORM_DEFAULTS)
        badValue |= !(vxi->SetRuntimeProperty(VXI::PlatDefaultsURI, value));
      else if (keyString == VXI_DEFAULT_ACCESS_CONTROL)
        badValue |= !(vxi->SetRuntimeProperty(VXI::DefaultAccessControl, value));
      else
        badName = true;
	}
  } while (!badName && !badValue && (VXIMapGetNextProperty(i, &key, &value) == VXIvalue_RESULT_SUCCESS));

  VXIMapIteratorDestroy(&i);

  if (badName) 
    return VXIinterp_RESULT_INVALID_PROP_NAME;
  if (badValue)
    return VXIinterp_RESULT_INVALID_PROP_VALUE;
  return VXIinterp_RESULT_SUCCESS;
}


extern "C" VXIinterpreterResult VXIinterpreterValidate(
                                                 VXIinterpreterInterface *x,
                                                 const VXIchar *name)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;
  if (name == NULL) return VXIinterp_RESULT_FETCH_ERROR;

  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  //  Overload this function for purposes of VXMLDocument frees
  std::string addr_string = VXIchar_to_Std_String (name);
  if (addr_string.find("0x") == 0)
  {
      //  Handle as a VXMLDocument free
      VXMLDocument *doc =
	  (VXMLDocument *) Std_String_to_Pointer (addr_string);
      if (voiceglue_loglevel() >= LOG_DEBUG)
      {
	  std::ostringstream logstring;
	  logstring << "Validate free VXMLDOcument "
		    << Pointer_to_Std_String ((void *) doc);
	  voiceglue_log ((char) LOG_DEBUG, logstring);
      };
      delete doc;
      return VXIinterp_RESULT_SUCCESS;
  };

  DocumentParser parser;
  VXIMapHolder properties;
  VXMLDocument document;
  VXIMapHolder docProperties;

  switch (parser.FetchDocument(name, properties, interpreter->inet,
                               interpreter->cache, *interpreter->logger,
                               document, docProperties))
  {
  case -1: // Out of memory?
    return VXIinterp_RESULT_OUT_OF_MEMORY;
  case  0: // Success
    return VXIinterp_RESULT_SUCCESS;
  case  2: // Unable to open URI
  case  3: // Unable to read from URI
    return VXIinterp_RESULT_FETCH_ERROR;
  case  4: // Unable to parse contents of URI
    return VXIinterp_RESULT_FAILURE;
  case  1: // Invalid parameter
  default:
    break;
  }

  return VXIinterp_RESULT_FATAL_ERROR;
}


extern "C" VXIinterpreterResult
VXIinterpreterRequestStop(VXIinterpreterInterface *x, VXIbool doStop)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;
  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  interpreter->implementation->DeclareStopRequest(doStop == TRUE);

  return VXIinterp_RESULT_SUCCESS;
}


extern "C" VXIinterpreterResult VXIinterpreterInsertEvent(
                                                    VXIinterpreterInterface *x,
                                                    const VXIchar * event,
                                                    const VXIchar * message)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;
  if (event == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;

  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  bool res = interpreter->implementation->DeclareExternalEvent(event, message);
  return res ? VXIinterp_RESULT_SUCCESS : VXIinterp_RESULT_INVALID_ARGUMENT;
}


extern "C" VXIinterpreterResult VXIinterpreterClearEventQueue(
                                                    VXIinterpreterInterface *x)
{
  if (x == NULL) return VXIinterp_RESULT_INVALID_ARGUMENT;

  Interpreter * interpreter = reinterpret_cast<Interpreter*>(x);

  bool res = interpreter->implementation->ClearExternalEventQueue();
  return res ? VXIinterp_RESULT_SUCCESS : VXIinterp_RESULT_FATAL_ERROR;
}

/******************************************************************
 * Resource creation & destruction
 ******************************************************************/

// VXIinterp_RESULT_OUT_OF_MEMORY
// VXIinterp_RESULT_SUCCESS
VXI_INTERPRETER
VXIinterpreterResult VXIinterpreterCreateResource(VXIresources *resources,
                                                  VXIinterpreterInterface ** v)
{
  if (resources == NULL || resources->inet == NULL || 
      resources->log == NULL || v == NULL)
    return VXIinterp_RESULT_INVALID_ARGUMENT;

  Interpreter * interpreter = new Interpreter;
  if (interpreter == NULL)
    return VXIinterp_RESULT_OUT_OF_MEMORY;

  interpreter->implementation = new VXI();
  if (interpreter->implementation == NULL) {
    delete interpreter;
    return VXIinterp_RESULT_OUT_OF_MEMORY;
  }

  interpreter->logger = SimpleLogger::CreateResource(resources->log);
  if (interpreter->logger == NULL) {
    delete interpreter->implementation;
    delete interpreter;
    return VXIinterp_RESULT_OUT_OF_MEMORY;
  }

  interpreter->interface.GetVersion = VXIinterpreterGetVersion;
  interpreter->interface.GetImplementationName = 
    VXIinterpreterGetImplementationName;
  interpreter->interface.Run = VXIinterpreterRun;
  interpreter->interface.Validate = VXIinterpreterValidate;
  interpreter->interface.SetProperties = VXIinterpreterSetProperties;
  interpreter->interface.RequestStop = VXIinterpreterRequestStop;
  interpreter->interface.InsertEvent = VXIinterpreterInsertEvent;
  interpreter->interface.ClearEventQueue = VXIinterpreterClearEventQueue;

  // The VXI requires a 1.1 cache interface version.
  if (resources->cache != NULL && resources->cache->GetVersion() >= 0x00010001)
    interpreter->cache = resources->cache;
  else
    interpreter->cache = NULL;

  interpreter->inet   = resources->inet;
  interpreter->jsi    = resources->jsi;
  interpreter->object = resources->object;
  interpreter->prompt = resources->prompt;
  interpreter->rec    = resources->rec;
  interpreter->tel    = resources->tel;

  *v = reinterpret_cast<VXIinterpreterInterface*>(interpreter);
  return VXIinterp_RESULT_SUCCESS;
}


VXI_INTERPRETER
void VXIinterpreterDestroyResource(VXIinterpreterInterface ** v)
{
  if (v == NULL || *v == NULL) return;
  Interpreter * interpreter = reinterpret_cast<Interpreter*>(*v);
  delete interpreter->implementation;
  SimpleLogger::DestroyResource(interpreter->logger);
  delete interpreter;
  *v = NULL;
}
