
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

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

#include <string.h>                     // For memset( )
#include <stdio.h>                      // For fopen( ), fclose( ), fwrite( )
#include <errno.h>                      // For errno

#include <VXIlog.h>                     // For VXIlog interface
#include "VXIobjectAPI.h"               // Header for these functions
#include "VXIclient.h"

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif
static const VXIchar MODULE_VXIOBJECT[]              = MODULE_PREFIX L"VXIobject";
static const VXIchar VXIOBJECT_IMPLEMENTATION_NAME[] = COMPANY_DOMAIN L".VXIobject";

// Global variable to track whether this is initialized
static bool gblInitialized = false;

// Global diagnostic logging base
static VXIunsigned gblDiagLogBase;

// Diagnostic logging tags
static const VXIunsigned LOG_API = 0;

// VXIobject interface, "inherits" from VXIobjectInterface
typedef struct VXIobjectAPI
{
  // Base interface, must be the first member
  VXIobjectInterface  object;

  // Resources for this channel
  VXIobjectResources  *resources;

} VXIobjectAPI;

// Convenience macro
#define GET_VXIOBJECT(pThis, vxiObject, log, rc) \
  VXIobjResult rc = VXIobj_RESULT_SUCCESS; \
  VXIobjectAPI *objectAPI = (VXIobjectAPI *) pThis; \
  if ( ! objectAPI ) { rc = VXIobj_RESULT_INVALID_ARGUMENT; return rc; } \
  VXIlogInterface *log = objectAPI->resources->log;


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

/**
 * Internal function for error logging
 */
static VXIlogResult
Error (VXIlogInterface *log, VXIunsigned errorID, const VXIchar *format, ...)
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;
  
  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VError)(log, MODULE_VXIOBJECT, errorID, format, arguments);
    va_end(arguments);
  } else {
    rc = (*log->Error)(log, MODULE_VXIOBJECT, errorID, NULL);
  }

  return rc;
}

/**
 * Internal function for diagnostic logging
 */
static VXIlogResult
Diag (VXIlogInterface *log, VXIunsigned tag, const VXIchar *subtag,
      const VXIchar *format, ...)
{
  va_list arguments;

  if ( ! log )
    return VXIlog_RESULT_FAILURE;
  
  VXIlogResult rc;
  if ( format ) {
    va_start(arguments, format);
    rc = (*log->VDiagnostic)(log, tag + gblDiagLogBase, subtag, format,
           arguments);
    va_end(arguments);
  } else {
    rc = (*log->Diagnostic)(log, tag + gblDiagLogBase, subtag, NULL);
  }

  return rc;
}

static void ShowPropertyValues(const VXIchar *key,
           const VXIValue *value, VXIunsigned PROP_TAG,
           VXIlogInterface *log)
{
  VXIchar subtag[512] = L"Property";
  if( value == 0 ) return;
  VXIvalueType type = VXIValueGetType(value);
  {
    switch( type )
    {
      case VALUE_INTEGER:
        wcscpy(subtag, L"Property:INT");
        Diag(log, PROP_TAG, subtag, 
             L"%s=%d", key, VXIIntegerValue((const VXIInteger*)value));
        break;
      case VALUE_FLOAT:
        wcscpy(subtag, L"Property:FLT");
        Diag(log, PROP_TAG, subtag, 
             L"%s=%f", key, VXIFloatValue((const VXIFloat*)value));
        break;
      case VALUE_BOOLEAN:     
        wcscpy(subtag, L"Property:BOOL");
        Diag(log, PROP_TAG, subtag, 
             L"%s=%d", key, VXIBooleanValue((const VXIBoolean*)value));
        break;
      case VALUE_STRING:
        wcscpy(subtag, L"Property:STR");
        Diag(log, PROP_TAG, subtag, 
             L"%s=%s", key, VXIStringCStr((const VXIString*)value));
        break;
      case VALUE_PTR:
        wcscpy(subtag, L"Property:PTR");
        Diag(log, PROP_TAG, subtag, 
             L"%s(ptr)=0x%p", key, VXIPtrValue((const VXIPtr*)value));
        break;
      case VALUE_CONTENT:
        wcscpy(subtag, L"Property:CNT");
        Diag(log, PROP_TAG, subtag, 
             L"%s(content)=0x%p", key, value);
        break;
      case VALUE_MAP:
        {
          VXIchar endtag[512];
          const VXIchar *mykey = key ? key : L"NULL";
          wcscpy(subtag, L"Property:MAP:BEG");
          wcscpy(endtag, L"Property:MAP:END");
          Diag(log, PROP_TAG, subtag, L"%s", mykey);
          const VXIchar *key = NULL;
          const VXIValue *gvalue = NULL;

          VXIMapIterator *it = VXIMapGetFirstProperty((const VXIMap*)value, &key, &gvalue);
          int ret = 0;
          while( ret == 0 && key && gvalue )
          {
            ShowPropertyValues(key, gvalue, PROP_TAG, log);
            ret = VXIMapGetNextProperty(it, &key, &gvalue);
          }
          VXIMapIteratorDestroy(&it);         
          Diag(log, PROP_TAG, endtag, L"%s", mykey);
        }   
        break;
      case VALUE_VECTOR:
        {
          VXIunsigned vlen = VXIVectorLength((const VXIVector*)value);
          for(VXIunsigned i = 0; i < vlen; ++i)
          {
            const VXIValue *gvalue = VXIVectorGetElement((const VXIVector*)value, i);
            ShowPropertyValues(L"Vector", gvalue, PROP_TAG, log);
          }
        }
        break;
      default:
        Diag(log, PROP_TAG, subtag, L"%s=%s", key, L"UNKOWN");
    }          
  }
  return;
}

static void ShowPropertyValue(const VXIMap *properties, VXIlogInterface *log)
{
  const VXIchar *key = NULL;
  const VXIValue *gvalue = NULL;
  const VXIunsigned PROP_TAG = 5;
  if( log && log->DiagnosticIsEnabled(log, PROP_TAG+gblDiagLogBase) && properties) {
    VXIMapIterator *it = VXIMapGetFirstProperty(properties, &key, &gvalue);
    int ret = 0;
    while( ret == 0 && key && gvalue )
    {
      ShowPropertyValues(key, gvalue, PROP_TAG, log);
      ret = VXIMapGetNextProperty(it, &key, &gvalue);
    }
    VXIMapIteratorDestroy(&it);     
  }
  return;
}


/**
 * Sample diagnostic logging object
 *
 * @param properties    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param parameters    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param execute       [IN]  Specifies whether the object should be
 *                            executed (true) or simply validated (false)
 * @param result        [OUT] See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
static VXIobjResult 
ProcessComSpeechworksDiagObject (struct VXIobjectInterface *pThis,
                                 const VXIMap              *properties,
                                 const VXIMap              *parameters,
                                 VXIbool                    execute,
         VXIValue                 **result)
{
  static const wchar_t func[] = L"ProcessComSpeechworksDiagObject";
  GET_VXIOBJECT (pThis, sbObject, log, rc);

  if((execute) && (result == NULL)) return VXIobj_RESULT_INVALID_ARGUMENT;
  if((! properties) || (! parameters)) return VXIobj_RESULT_INVALID_ARGUMENT;

  // Get the tag ID
  const VXIValue *val = VXIMapGetProperty(parameters, L"tag");
  if(val == NULL) {
    Error(log, 202, L"%s%s", L"parameter", L"tag");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } 
  VXIint tag;
  switch (VXIValueGetType(val)) {
  case VALUE_INTEGER:
    tag = VXIIntegerValue((VXIInteger *)val);
    break;
  case VALUE_STRING: {
    wchar_t *ptr;
    tag = ::wcstol(VXIStringCStr((VXIString *)val), &ptr, 10);
    } break;
  default:
    Error(log, 203, L"%s%s%s%d", L"parameter", L"tag", 
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }

  // Get the message string
  val = VXIMapGetProperty(parameters, L"message");
  if(val == NULL) {
    Error(log, 202, L"%s%s", L"parameter", L"message");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }

  // Check whether the message was sent in "data" or "ref" format.
  // If it is "ref", we need to retrieve it in the embedded map.
  const VXIchar *messageStr = NULL;
  switch (VXIValueGetType(val)) {
  case VALUE_MAP: {
    const VXIValue *val2 = VXIMapGetProperty((const VXIMap *)val, 
                                             OBJECT_VALUE);
    if (VXIValueGetType(val2) == VALUE_STRING)
      messageStr = VXIStringCStr((VXIString *)val2);
    } break;
  case VALUE_STRING:
    messageStr = VXIStringCStr((VXIString *)val);
    break;
  default:
    Error(log, 203, L"%s%s%s%d", L"parameter", L"message", 
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }

  if ((! messageStr) || (! messageStr[0])) {
    Error(log, 204, L"%s%s", L"parameter", L"message");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  
  if(execute) {
    // Print a diagnostic message using the retrieved arguments.
    // To see this message, you must enable client.log.diagTag.xxx
    // in your VXIclient configuration file, where 'xxx' is the 
    // value of client.object.diagLogBase defined in the same file.
    VXIlogResult rc = Diag (log, tag, NULL, messageStr);

    // Create the result object
    VXIMap *resultObj = VXIMapCreate();
    if(resultObj == NULL) {
      Error(log, 100, NULL);
      return VXIobj_RESULT_OUT_OF_MEMORY;
    }
    *result = reinterpret_cast<VXIValue *>(resultObj);

    // Set the result object's status field to 'success' or 'failure'
    if(rc == VXIlog_RESULT_SUCCESS)
      VXIMapSetProperty(resultObj, L"status",
               reinterpret_cast<VXIValue *>(VXIStringCreate(L"success")));
    else
      VXIMapSetProperty(resultObj, L"status", 
               reinterpret_cast<VXIValue *>(VXIStringCreate(L"failure")));
  }

  return VXIobj_RESULT_SUCCESS;
}

/**
 * Sample object echoing back all parameters
 *
 * @param properties    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param parameters    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param execute       [IN]  Specifies whether the object should be
 *                            executed (true) or simply validated (false)
 * @param result        [OUT] See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
static VXIobjResult 
ProcessComSpeechworksEchoObject (struct VXIobjectInterface *pThis,
                                 const VXIMap              *properties,
                                 const VXIMap              *parameters,
                                 VXIbool                    execute,
         VXIValue                 **result)
{
  static const wchar_t func[] = L"ProcessComSpeechworksEchoObject";
  GET_VXIOBJECT (pThis, sbObject, log, rc);

  if((execute) && (result == NULL)) return VXIobj_RESULT_INVALID_ARGUMENT;
  if((! properties) || (! parameters)) return VXIobj_RESULT_INVALID_ARGUMENT;

  if(execute) {
    // Create the result object
    VXIMap *resultObj = VXIMapCreate();
    if(resultObj == NULL) {
      Error(log, 100, NULL);
      return VXIobj_RESULT_OUT_OF_MEMORY;
    }
    *result = reinterpret_cast<VXIValue *>(resultObj);

    // Simply add the input properties and parameters to the result object
    VXIMapSetProperty(resultObj, L"attributes", 
          VXIValueClone((VXIValue *)properties));
    VXIMapSetProperty(resultObj, L"parameters", 
          VXIValueClone((VXIValue *)parameters));

  }

  return VXIobj_RESULT_SUCCESS;
}

/**
 * Sample object to save a recording to a file
 *
 * @param properties    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param parameters    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param execute       [IN]  Specifies whether the object should be
 *                            executed (true) or simply validated (false)
 * @param result        [OUT] See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
static VXIobjResult 
ProcessComSpeechworksSaveRecordingObject (struct VXIobjectInterface *pThis,
            const VXIMap             *properties,
            const VXIMap             *parameters,
            VXIbool                    execute,
            VXIValue                 **result)
{
  static const wchar_t func[] = L"ProcessComSpeechworksSaveRecordingObject";
  GET_VXIOBJECT (pThis, sbObject, log, rc);

  if((execute) && (result == NULL)) return VXIobj_RESULT_INVALID_ARGUMENT;
  if((! properties) || (! parameters)) return VXIobj_RESULT_INVALID_ARGUMENT;

  // Get the recording, MIME type, size, and destination path
  const VXIbyte *recording = NULL;
  const VXIchar *type = NULL, *dest = NULL;
  VXIulong size = 0;
  const VXIValue *val = VXIMapGetProperty(parameters, L"recording");
  if (! val) {
    Error(log, 202, L"%s%s", L"parameter", L"recording");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } else if (VXIValueGetType(val) != VALUE_CONTENT) {
    Error(log, 203, L"%s%s%s%d", L"parameter", L"recording", 
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  if (VXIContentValue(reinterpret_cast<const VXIContent *>(val),
          &type, &recording, &size) != VXIvalue_RESULT_SUCCESS) {
    Error(log, 204, L"%s%s", L"parameter", L"recording");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } else if (size < 1) {
    Error(log, 204, L"%s%s%s%d", L"parameter", L"recording.size", 
    L"value", L"size");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  
  val = VXIMapGetProperty(parameters, L"dest");
  if (! val) {
    Error(log, 202, L"%s%s", L"parameter", L"dest");
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } else if (VXIValueGetType(val) != VALUE_STRING) {
    Error(log, 203, L"%s%s%s%d", L"parameter", L"dest", 
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  dest = VXIStringCStr(reinterpret_cast<const VXIString *>(val));
  if (! dest[0]) {
    Error(log, 204, L"%s%s%s%s", L"parameter", L"dest", L"value", dest);
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }

  if(execute) {
    // Convert the destination to narrow characters, use an upside
    // down question mark for unsupported Unicode characters
    size_t len = wcslen(dest);
    char *ndest = new char [len + 1];
    if (! ndest) {
      Error(log, 100, NULL);
      return VXIobj_RESULT_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i <= len; i++)
      ndest[i] = (dest[i] & 0xff00 ? '\277' : static_cast<char>(dest[i]));
    
    // Open the destination and save the file
    size_t totWritten = 0;
#ifdef VXIOBJECT_PERMIT_FILE_WRITES
    FILE *fp = fopen(ndest, "wb");
    delete [] ndest;
    if (fp) {
      do {
  size_t w = fwrite(&recording[totWritten], 1, 
        ((size_t) size) - totWritten, fp);
  totWritten += w;
      } while ((totWritten < (size_t) size) && (! ferror(fp)));
      
      fclose(fp);
    } else {
      Error(log, 205, L"%s%s%s%d", L"file", dest, L"errno", errno);
    }
#else
    Error(log, 206, L"%s%s", L"file", dest);
#endif

    // Create the result object
    VXIMap *resultObj = VXIMapCreate();
    if(resultObj == NULL) {
      Error(log, 100, NULL);
      return VXIobj_RESULT_OUT_OF_MEMORY;
    }
    *result = reinterpret_cast<VXIValue *>(resultObj);

    // Set the result object's status field to 'success' or 'failure'
    if (totWritten == (size_t) size)
      VXIMapSetProperty(resultObj, L"status",
               reinterpret_cast<VXIValue *>(VXIStringCreate(L"success")));
    else
      VXIMapSetProperty(resultObj, L"status", 
               reinterpret_cast<VXIValue *>(VXIStringCreate(L"failure")));
  }

  return VXIobj_RESULT_SUCCESS;
}

/**
 * Sample object setting the defaults document for subsequent calls
 *
 * @param properties    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param parameters    [IN]  See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 * @param execute       [IN]  Specifies whether the object should be
 *                            executed (true) or simply validated (false)
 * @param result        [OUT] See description in VXIobjectExecute() 
 *                            or VXIobjectValidate()
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
static VXIobjResult 
ProcessComSpeechworksSetDefaultsObject (struct VXIobjectInterface *pThis,
					const VXIMap              *properties,
					const VXIMap              *parameters,
					VXIbool                    execute,
					VXIValue                 **result)
{
  static const wchar_t func[] = L"ProcessComSpeechworksSetDefaults";
  GET_VXIOBJECT (pThis, sbObject, log, rc);

  if((execute) && (result == NULL)) return VXIobj_RESULT_INVALID_ARGUMENT;
  if((! properties) || (! parameters)) return VXIobj_RESULT_INVALID_ARGUMENT;

  if(execute) {

    const VXIchar *defaults = NULL;

    const VXIValue *val = VXIMapGetProperty(parameters, L"defaults");
    if (! val) {
      Error(log, 202, L"%s%s", L"parameter", L"defaults");
      return VXIobj_RESULT_INVALID_PROP_VALUE;
    } else if (VXIValueGetType(val) != VALUE_STRING) {
      Error(log, 203, L"%s%s%s%d", L"parameter", L"defaults", 
	    L"type", VXIValueGetType(val));
      return VXIobj_RESULT_INVALID_PROP_VALUE;
    }
    defaults = VXIStringCStr(reinterpret_cast<const VXIString *>(val));
    if (! defaults[0]) {
      Error(log, 204, L"%s%s%s%s", L"parameter", L"defaults", L"value", defaults);
      return VXIobj_RESULT_INVALID_PROP_VALUE;
    }

    
    // Hack to set the defaults document
    VXIobjectAPI *objectAPI = (VXIobjectAPI *) pThis;
    if ( ! objectAPI ) { rc = VXIobj_RESULT_INVALID_ARGUMENT; return rc; }
    VXIplatform *plat = objectAPI->resources->platform;
    VXIMap *vxiProperties = VXIMapCreate();
    VXIString * valstr = VXIStringCreate(defaults);
    VXIMapSetProperty(vxiProperties, VXI_PLATFORM_DEFAULTS, (VXIValue *) valstr);
    plat->VXIinterpreter->SetProperties(plat->VXIinterpreter,
					vxiProperties);

    // Create the result object
    VXIMap *resultObj = VXIMapCreate();
    if(resultObj == NULL) {
      Error(log, 100, NULL);
      return VXIobj_RESULT_OUT_OF_MEMORY;
    }
    *result = reinterpret_cast<VXIValue *>(resultObj);

    // Set the result object's status field to 'success'
    VXIMapSetProperty(resultObj, L"status",
		      reinterpret_cast<VXIValue *>(VXIStringCreate(L"success")));
  }

  return VXIobj_RESULT_SUCCESS;
}

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

/**
 * @name GetVersion
 * @memo Get the VXI interface version implemented
 *
 * @return  VXIint32 for the version number. The high high word is 
 *          the major version number, the low word is the minor version 
 *          number, using the native CPU/OS byte order. The current
 *          version is VXI_CURRENT_VERSION as defined in VXItypes.h.
 */ 
static
VXIint32 VXIobjectGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}


/**
 * @name GetImplementationName
 * @memo Get the name of the implementation
 *
 * @return  Implementation defined string that must be different from
 *          all other implementations. The recommended name is one
 *          where the interface name is prefixed by the implementator's
 *          Internet address in reverse order, such as com.xyz.rec for
 *          VXIobject from xyz.com. This is similar to how VoiceXML 1.0
 *          recommends defining application specific error types.
 */
static
const VXIchar* VXIobjectGetImplementationName(void)
{
  return VXIOBJECT_IMPLEMENTATION_NAME;
}


/**
 * Execute an object
 *
 * @param properties  [IN] Map containing properties and attributes for
 *                      the <object> as specified above.
 * @param parameters  [IN] Map containing parameters for the <object> as
 *                      specified by the VoiceXML <param> tag. The keys
 *                      of the map correspond to the parameter name ("name"
 *                      attribute) while the value of each key corresponds
 *                      to a VXIValue based type.
 *
 *                      For each parameter, any ECMAScript expressions are
 *                      evaluated by the interpreter. Then if the "valuetype"
 *                      attribute is set to "ref" the parameter value is
 *                      packaged into a VXIMap with three properties:
 *
 *                      OBJECT_VALUE:       actual parameter value
 *                      OBJECT_VALUETYPE:   "valuetype" attribute value
 *                      OBJECT_TYPE:        "type" attribute value
 *
 *                      Otherwise a primitive VXIValue based type will
 *                      be used to specify the value.
 * @param result      [OUT] Return value for the <object> execution, this
 *                      is allocated on success, the caller is responsible
 *                      for destroying the returned value by calling 
 *                      VXIValueDestroy( ). The object's field variable
 *                      will be set to this value.
 *
 * @return        VXIobj_RESULT_SUCCESS on success,
 *                VXIobj_RESULT_NON_FATAL_ERROR on error, 
 *                VXIobj_RESULT_UNSUPPORTED for unsupported object types
 *                 (this will cause interpreter to throw the correct event)
 */
static
VXIobjResult VXIobjectExecute(struct VXIobjectInterface *pThis,
                              const VXIMap              *properties,
                              const VXIMap              *parameters,
                              VXIValue                  **result)
{
  static const wchar_t func[] = L"VXIobjectExecute";
  GET_VXIOBJECT (pThis, sbObject, log, rc);
  Diag (log, LOG_API, func, L"entering: 0x%p, 0x%p, 0x%p, 0x%p", 
  pThis, properties, parameters, result);
  
  if(properties == NULL) {
    Error (log, 200, NULL);
    return VXIobj_RESULT_INVALID_ARGUMENT;
  }

  ShowPropertyValue(properties, log);

  // Get the name of the object to execute
  const VXIValue *val = VXIMapGetProperty(properties, OBJECT_CLASS_ID);
  if(val == NULL) {
    Error(log, 202, L"%s%s", L"parameter", OBJECT_CLASS_ID);
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } else if (VXIValueGetType(val) != VALUE_STRING) {
    Error(log, 203, L"%s%s%s%d", L"parameter", OBJECT_CLASS_ID,
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  const VXIchar *classID = VXIStringCStr((VXIString *)val);

  // Handle the object
  if (::wcscmp(classID, L"com.vocalocity.diag") == 0) {
    //
    // Sample diagnostic logging object.
    // 
    rc = ProcessComSpeechworksDiagObject(pThis, properties, parameters, true,
           result);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else if (::wcscmp(classID, L"com.vocalocity.echo") == 0) {
    //
    // Sample object echoing back all attributes and parameters
    // 
    rc = ProcessComSpeechworksEchoObject(pThis, properties, parameters, true,
           result);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else if (::wcscmp(classID, L"com.vocalocity.saveRecording") == 0) {
    //
    // Sample object that saves a recording to a file
    // 
    rc = ProcessComSpeechworksSaveRecordingObject(pThis, properties, 
              parameters, true, result);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else if (::wcscmp(classID, L"com.vocalocity.setDefaults") == 0) {
    //
    // Sample object that sets the defaults document for subsequent
    // calls. This is really a hack and not recommended. This was
    // done more for fun than any value.
    // 
    rc = ProcessComSpeechworksSetDefaultsObject(pThis, properties, 
              parameters, true, result);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else {
    //
    // Unsupported object
    //
    rc = VXIobj_RESULT_UNSUPPORTED;
  }

  Diag (log, LOG_API, func, L"exiting: returned %d", rc);
  return rc;
}


/**
 * Validate an object, performing validity checks without execution
 *
 * @param properties  [IN] Map containing properties and attributes for
 *                      the <object> as specified in the VoiceXML
 *                      specification except that "expr" and "cond" are
 *                      always omitted (are handled by the interpreter).
 * @param parameters  [IN] Map containing parameters for the <object> as
 *                      specified by the VoiceXML <param> tag. The keys
 *                      of the map correspond to the parameter name ("name"
 *                      attribute) while the value of each key corresponds
 *                      to a VXIValue based type. See Execute( ) above 
 *                      for details.
 *
 * @return        VXIobj_RESULT_SUCCESS on success,
 *                VXIobj_RESULT_NON_FATAL_ERROR on error, 
 *                VXIobj_RESULT_UNSUPPORTED for unsupported object types
 *                 (this will cause interpreter to throw the correct event)
 */
static
VXIobjResult VXIobjectValidate(struct VXIobjectInterface *pThis,
                               const VXIMap              *properties,
                               const VXIMap              *parameters)
{
  static const wchar_t func[] = L"VXIobjectValidate";
  GET_VXIOBJECT (pThis, sbObject, log, rc);
  Diag (log, LOG_API, func, L"entering: 0x%p, 0x%p, 0x%p", 
  pThis, properties, parameters);

  if(properties == NULL) {
    Error (log, 201, NULL);
    return VXIobj_RESULT_INVALID_ARGUMENT;
  }

  // Get the name of the object to execute
  const VXIValue *val = VXIMapGetProperty(properties, OBJECT_CLASS_ID);
  if(val == NULL) {
    Error(log, 202, L"%s%s", L"parameter", OBJECT_CLASS_ID);
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  } else if (VXIValueGetType(val) != VALUE_STRING) {
    Error(log, 203, L"%s%s%s%d", L"parameter", OBJECT_CLASS_ID,
    L"type", VXIValueGetType(val));
    return VXIobj_RESULT_INVALID_PROP_VALUE;
  }
  const VXIchar *classID = VXIStringCStr((VXIString *)val);

  // Handle the object
  if (::wcscmp(classID, L"com.vocalocity.diag") == 0) {
    //
    // Sample diagnostic logging object
    // 
    rc = ProcessComSpeechworksDiagObject(pThis, properties, parameters, false,
           NULL);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  }
  else if (::wcscmp(classID, L"com.vocalocity.echo") == 0) {
    //
    // Sample object echoing back all attributes and parameters
    // 
    rc = ProcessComSpeechworksEchoObject(pThis, properties, parameters, false,
           NULL);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else if (::wcscmp(classID, L"com.vocalocity.saveRecording") == 0) {
    //
    // Sample object that saves a recording to a file
    // 
    rc = ProcessComSpeechworksSaveRecordingObject(pThis, properties, 
              parameters, false, NULL);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else if (::wcscmp(classID, L"com.vocalocity.setDefaults") == 0) {
    //
    // Sample object that sets the defaults document for subsequent
    // calls. This is really a hack and not recommended. This was
    // done more for fun than any value.
    // 
    rc = ProcessComSpeechworksSetDefaultsObject(pThis, properties, 
              parameters, false, NULL);
    if(rc != VXIobj_RESULT_SUCCESS) rc = VXIobj_RESULT_NON_FATAL_ERROR;
  } 
  else {
    //
    // Unsupported object
    //
    rc = VXIobj_RESULT_UNSUPPORTED;
  }

  Diag (log, LOG_API, func, L"exiting: returned %d", rc);
  return rc;
}

// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

/**
 * Global platform initialization of VXIobject
 *
 * @param log            VXI Logging interface used for error/diagnostic 
 *                       logging, only used for the duration of this 
 *                       function call
 * @param  diagLogBase   Base tag number for diagnostic logging purposes.
 *                       All diagnostic tags for VXIobject will start at this
 *                       ID and increase upwards.
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
VXIOBJECT_API_EX VXIobjResult 
VXIobjectInit (VXIlogInterface  *log,
               VXIunsigned       diagLogBase)
{
  static const wchar_t func[] = L"VXIobjectInit";
  gblDiagLogBase = diagLogBase;
  Diag (log, LOG_API, func, L"entering: 0x%p, %u", log, diagLogBase);

  VXIobjResult rc = VXIobj_RESULT_SUCCESS;
  if (gblInitialized == true) {
    rc = VXIobj_RESULT_FATAL_ERROR;
    Error (log, 101, NULL);
    Diag (log, LOG_API, func, L"exiting: returned %d", rc);
    return rc;
  }

  if ( ! log )
    return VXIobj_RESULT_INVALID_ARGUMENT;

  // Return
  gblInitialized = true;

  Diag (log, LOG_API, func, L"exiting: returned %d", rc);
  return rc;
}

/**
 * Global platform shutdown of VXIobject
 *
 * @param log    VXI Logging interface used for error/diagnostic logging,
 *               only used for the duration of this function call
 *
 * @result VXIobj_RESULT_SUCCESS on success
 */
VXIOBJECT_API_EX VXIobjResult 
VXIobjectShutDown (VXIlogInterface  *log)
{
  static const wchar_t func[] = L"VXIobjectShutDown";
  Diag (log, LOG_API, func, L"entering: 0x%p",log);

  VXIobjResult rc = VXIobj_RESULT_SUCCESS;
  if (gblInitialized == false) {
    rc = VXIobj_RESULT_FATAL_ERROR;
    Error (log, 102, NULL);
    Diag (log, LOG_API, func, L"exiting: returned %d", rc);
    return rc;
  }

  if ( ! log )
    return VXIobj_RESULT_INVALID_ARGUMENT;

  gblInitialized = false;

  Diag (log, LOG_API, func, L"exiting: returned %d", rc);
  return rc;
}

/**
 * Create a new object service handle
 *
 * @param resources  A pointer to a structure containing all the interfaces
 *                   that may be required by the object resource
 *
 * @result VXIobj_RESULT_SUCCESS on success 
 */
VXIOBJECT_API_EX VXIobjResult 
VXIobjectCreateResource(VXIobjectResources   *resources,
                        VXIobjectInterface   **object)
{
  static const wchar_t func[] = L"VXIobjectCreateResource";
  VXIlogInterface *log = NULL;
  if (( resources ) && ( resources->log )) {
    log = resources->log;
    Diag (log, LOG_API, func, L"entering: 0x%p, 0x%p", resources, object);
  }

  VXIobjResult rc = VXIobj_RESULT_SUCCESS;
  if (gblInitialized == false) {
    rc = VXIobj_RESULT_FATAL_ERROR;
    Error (log, 102, NULL);
    Diag (log, LOG_API, func, L"exiting: returned %d, 0x%p",
    rc, (object ? *object : NULL));
    return rc;
  }

  if (( ! log ) || ( ! resources ) || ( ! object )) {
    rc = VXIobj_RESULT_INVALID_ARGUMENT;
    Error (log, 103, NULL);
    Diag (log, LOG_API, func, L"exiting: returned %d, 0x%p",
    rc, (object ? *object : NULL));
    return rc;
  }

  *object = NULL;

  // Get a new interface instance
  VXIobjectAPI *newObject = new VXIobjectAPI;
  if (newObject == false) {
    rc = VXIobj_RESULT_OUT_OF_MEMORY;
    Error (log, 100, NULL);
    Diag (log, LOG_API, func, L"exiting: returned %d, 0x%p", rc, *object);
    return rc;
  }
  memset (newObject, 0, sizeof (VXIobjectAPI));

  // Initialize the function pointers
  newObject->object.GetVersion            = VXIobjectGetVersion;
  newObject->object.GetImplementationName = VXIobjectGetImplementationName;
  newObject->object.Execute               = VXIobjectExecute;
  newObject->object.Validate              = VXIobjectValidate;

  // Initialize data members
  newObject->resources = resources;

  // Return the object
  if ( rc != VXIobj_RESULT_SUCCESS ) {
    if ( newObject )
      delete newObject;
  } else {
    *object = &(newObject->object);
  }

  Diag (log, LOG_API, func, L"exiting: returned %d, 0x%p", rc, *object);
  return rc;
}

/**
 * Destroy the interface and free internal resources. Once this is
 *  called, the resource interfaces passed to VXIobjectCreateResource( )
 *  may be released as well.
 *
 * @result VXIobj_RESULT_SUCCESS on success 
 */
VXIOBJECT_API_EX VXIobjResult
VXIobjectDestroyResource(VXIobjectInterface **object)
{
  VXIobjResult rc = VXIobj_RESULT_SUCCESS;
  static const wchar_t func[] = L"VXIobjectDestroyResource";
  // Can't log yet, don't have a log handle

  if (gblInitialized == false)
    return VXIobj_RESULT_FATAL_ERROR;

  if ((object == NULL) || (*object == NULL))
    return VXIobj_RESULT_INVALID_ARGUMENT;

  // Get the real underlying interface
  VXIobjectAPI *objectAPI = (VXIobjectAPI *) *object;

  VXIlogInterface *log = objectAPI->resources->log;
  Diag (log, LOG_API, func, L"entering: 0x%p (0x%p)", object, *object);

  // Delete the object
  delete objectAPI;
  *object = NULL;

  Diag (log, LOG_API, func, L"exiting: returned %d", rc);
  return rc;
}

