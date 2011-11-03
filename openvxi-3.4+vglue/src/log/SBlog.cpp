
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

#include "SBlogInternal.h"
#include <cstring>                  // For memset()
#include <ctime>                    // For time_t
#include <vector>                   // For STL vector template class

#define SBLOG_EXPORTS
#include "SBlog.h"                  // Header for these functions
#include "SBlogOSUtils.h"           // for SBlogGetTime(), SBlogVswprintf()

#ifdef __THREADED
#include "VXItrd.h"                 // for VXItrdMutex
#endif

static const VXIunsigned MAX_LOG_BUFFER  = 4096 * 2;
static const VXIunsigned TAG_ARRAY_SIZE = (((SBLOG_MAX_TAG) + 1) / 8 + 1);

#define SBLOG_ERR_OUT_OF_MEMORY 100, L"SBlog: Out of memory", NULL
#define SBLOG_ERR_ALREADY_INITIALIZED 104, L"SBlog: Already initialized", NULL
#define SBLOG_ERR_NOT_INITIALIZED 105, L"SBlog: Not initialized", NULL

#ifndef MODULE_PREFIX
#define MODULE_PREFIX  COMPANY_DOMAIN L"."
#endif
#define MODULE_NAME MODULE_PREFIX L"SBlog"

// Global variable to track whether this is initialized
static bool gblInitialized = false;


struct myAPI {
  SBlogInterface intf;
  void *impl_;
};


struct SBlogErrorCallbackData {
  SBlogErrorListener *callback;
  void *userdata;
  bool operator==(struct SBlogErrorCallbackData &other)
  { return (callback == other.callback && userdata == other.userdata); }  
  bool operator!=(struct SBlogErrorCallbackData &other)
  { return !operator==(other); }
};


struct SBlogDiagCallbackData {
  SBlogDiagnosticListener *callback;
  void *userdata;
  bool operator==(struct SBlogDiagCallbackData &other)
  { return (callback == other.callback && userdata == other.userdata); }
  bool operator!=(struct SBlogDiagCallbackData &other)
  { return !operator==(other); }
};


struct SBlogEventCallbackData{
  SBlogEventListener *callback;
  void *userdata;
  bool operator==(struct SBlogEventCallbackData &other)
  { return (callback == other.callback && userdata == other.userdata); }
  bool operator!=(struct SBlogEventCallbackData &other)
  { return !operator==(other); }
};


struct SBlogContentCallbackData{
  SBlogContentListener *callback;
  void *userdata;
  bool operator==(struct SBlogContentCallbackData &other)
  { return (callback == other.callback && userdata == other.userdata); }
  bool operator!=(struct SBlogContentCallbackData &other)
  { return !operator==(other); }
};


// Our definition of the opaque VXIlogStream
extern "C" {
struct VXIlogStream {
  // Linked list of underlying SBlogListener content streams
  typedef std::vector<SBlogStream *> STREAMS;
  STREAMS streams;
};
}


class SBlog {
public:
  SBlog(SBlogInterface *pThis);
  virtual ~SBlog();
  
  bool DiagnosticIsEnabled(VXIunsigned tagID);

  VXIlogResult DiagnosticLog(VXIunsigned    tagID,
                             const VXIchar* subtag,
                             const VXIchar* format,
                             va_list arguments) const;
  
  VXIlogResult EventLog(VXIunsigned            eventID,
                        const VXIchar*         format,
                        va_list arguments) const;

  VXIlogResult EventLog(VXIunsigned            eventID,
                        const VXIVector*       keys,
			const VXIVector*       values) const;

  VXIlogResult ErrorLog(const VXIchar*         moduleName,
                        VXIunsigned            errorID,
                        const VXIchar*         format,
                        va_list arguments) const;  

  VXIlogResult ContentOpen(const VXIchar*      moduleName,
			   const VXIchar*      contentType,
			   VXIString**         logKey,
			   VXIString**         logValue,
			   VXIlogStream**      stream) const;

  VXIlogResult ContentClose(VXIlogStream**     stream) const;

  VXIlogResult ContentWrite(const VXIbyte*     buffer,
			    VXIulong           buflen,
			    VXIulong*          nwritten,
			    VXIlogStream*      stream) const;
  
  VXIlogResult ControlDiagnosticTag(VXIunsigned tagID,
                                    VXIbool     state);

  // ----- Register/Unregister callback functions
  VXIlogResult RegisterErrorListener(SBlogErrorCallbackData *info);

  SBlogErrorCallbackData*
    UnregisterErrorListener(SBlogErrorCallbackData *info);
  
  VXIlogResult RegisterDiagnosticListener(SBlogDiagCallbackData *info);

  SBlogDiagCallbackData*
    UnregisterDiagnosticListener(SBlogDiagCallbackData *info);

  VXIlogResult RegisterEventListener(SBlogEventCallbackData *info);

  SBlogEventCallbackData*
    UnregisterEventListener(SBlogEventCallbackData *info);  

  VXIlogResult RegisterContentListener(SBlogContentCallbackData *info);

  SBlogContentCallbackData*
    UnregisterContentListener(SBlogContentCallbackData *info);  

  // ----- Internal error logging functions
  VXIlogResult Error(VXIunsigned errorID, const VXIchar *errorIDText,
		     const VXIchar *format, ...) const;
  static VXIlogResult GlobalError(VXIunsigned errorID, 
				  const VXIchar *errorIDText,
				  const VXIchar *format, ...);

private:
  // Internal methods
  static unsigned testbit(unsigned char num, int bitpos);
  static void setbit(unsigned char *num, int bitpos);
  static void clearbit(unsigned char *num, int bitpos);

  bool Convert2Index(VXIunsigned tagID,
		     VXIunsigned *index, 
		     VXIunsigned *bit_pos) const;

  VXIlogResult ParseKeyValue(const VXIchar *format,
			     va_list args,
			     VXIVector *keys,
			     VXIVector *values) const;
  
#ifdef __THREADED
  inline bool Lock( ) const { 
    bool rc = (VXItrdMutexLock(_callbackLock) == VXItrd_RESULT_SUCCESS); 
    if (! rc) Error(102, L"SBlog: Mutex lock failed", NULL);
    return rc;
  }
  inline bool Unlock( ) const {
    bool rc = (VXItrdMutexUnlock(_callbackLock) == VXItrd_RESULT_SUCCESS); 
    if (! rc) Error(103, L"SBlog: Mutex unlock failed", NULL);
    return rc;
  }
  inline VXIthreadID GetThreadID( ) const {
    return VXItrdThreadGetID();
  }
#else
  inline bool Lock( ) const { return true; }
  inline bool Unlock( ) const { return true; }
  inline VXIthreadID GetThreadID( ) const { return (VXIthreadID) 1; }
#endif

private:
  typedef std::vector<SBlogDiagCallbackData *> DIAGCALLBACKS;
  DIAGCALLBACKS diagCallbacks;

  typedef std::vector<SBlogErrorCallbackData *> ERRORCALLBACKS;
  ERRORCALLBACKS errorCallbacks;

  typedef std::vector<SBlogEventCallbackData *> EVENTCALLBACKS;
  EVENTCALLBACKS eventCallbacks;

  typedef std::vector<SBlogContentCallbackData *> CONTENTCALLBACKS;
  CONTENTCALLBACKS contentCallbacks;

#ifdef __THREADED
  VXItrdMutex *_callbackLock;
  VXItrdMutex *_internalErrorLoggingLock;
#endif

  SBlogInterface *_pThis;
  VXIthreadID _internalErrorLoggingThread;
  unsigned char _tagIDs[TAG_ARRAY_SIZE];
};


SBlog::SBlog(SBlogInterface *pThis) : 
#ifdef __THREADED
  _callbackLock(NULL), 
  _internalErrorLoggingLock(NULL), 
#endif
  _pThis(pThis),
  _internalErrorLoggingThread((VXIthreadID) -1)
{
  // reset TAG ID range
  memset(_tagIDs, 0, TAG_ARRAY_SIZE);

#ifdef __THREADED
  // Create the mutexes
  if ((VXItrdMutexCreate(&_callbackLock) != VXItrd_RESULT_SUCCESS) ||
      (VXItrdMutexCreate(&_internalErrorLoggingLock) != VXItrd_RESULT_SUCCESS))
    Error(101, L"OSBlog: Mutex create failed", NULL);
#endif
}


SBlog::~SBlog() 
{
#ifdef __THREADED
  // Destroy the mutexes
  VXItrdMutexDestroy(&_callbackLock);
  VXItrdMutexDestroy(&_internalErrorLoggingLock);
#endif
}

/**
 * testbit
 * testbit returns the value of the given bit
 * 1 is set, 0 is clear
 */
unsigned SBlog::testbit(unsigned char num, int bitpos)
{ 
  return (num >> bitpos) & ~(~0 << 1); 
}

/**
 * setbit sets a given bit
 */
void SBlog::setbit(unsigned char *num, int bitpos)
{
  *num |= (1 << bitpos);
}

/**
 * clearbit clears a given bit
 */
void SBlog::clearbit(unsigned char *num, int bitpos)
{
  *num &= ~(1 << bitpos);
}


bool SBlog::Convert2Index(VXIunsigned tagID,
			  VXIunsigned *index, 
			  VXIunsigned *bit_pos) const
{
  // check for overflow TAG ID
  if (tagID > SBLOG_MAX_TAG) {
    Error(300, L"SBlog: Tag ID is too large", L"%s%u", L"tagID", tagID);
    return false;
  }

  // retrieving index for char array
  *index = tagID/8;  // 8 bits per char

  // retrieving bit position (bit range from 0-7)
  *bit_pos = tagID%8;

  return true;  // done
}


VXIlogResult SBlog::ControlDiagnosticTag(VXIunsigned tagID,
                                         VXIbool state)
{
  VXIunsigned bindex, bpos;
  if(!Convert2Index(tagID, &bindex, &bpos))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  if(state)
    setbit(&_tagIDs[bindex], bpos);
  else
    clearbit(&_tagIDs[bindex], bpos);
    
  return VXIlog_RESULT_SUCCESS;
}


VXIlogResult SBlog::RegisterErrorListener(SBlogErrorCallbackData *info)
{
   if (info == NULL) {
     Error(200, L"SBlog: Internal error in RegisterErrorListener(), NULL "
	   L"callback data", NULL);
     return VXIlog_RESULT_INVALID_ARGUMENT;
   }

   if (! Lock( )) {
     return VXIlog_RESULT_SYSTEM_ERROR;
   } else {
     errorCallbacks.push_back(info);

     if (! Unlock( ))
       return VXIlog_RESULT_SYSTEM_ERROR;
   }

   return VXIlog_RESULT_SUCCESS;  
}


SBlogErrorCallbackData*
SBlog::UnregisterErrorListener(SBlogErrorCallbackData *info)
{
  if (info == NULL) {
    Error(201, L"SBlog: Internal error in UnregisterErrorListener(), NULL "
	  L"callback data", NULL);
    return NULL;
  }

  SBlogErrorCallbackData *status = NULL;
  if (! Lock( )) {
    return NULL;
  } else {
    for (ERRORCALLBACKS::iterator i = errorCallbacks.begin();
	 i != errorCallbacks.end(); ++i)
      {
	if (*info != *(*i)) continue;
	status = *i;
	errorCallbacks.erase(i);
	break;
      }

    if (! Unlock( ))
      return NULL;
  }

  return status; 
}


VXIlogResult SBlog::RegisterDiagnosticListener(SBlogDiagCallbackData *info)
{
   if (info == NULL) {
     Error(202, L"SBlog: Internal error in RegisterDiagnosticListener(), NULL "
	   L"callback data", NULL);
     return VXIlog_RESULT_INVALID_ARGUMENT;
   }

   if (! Lock( )) {
     return VXIlog_RESULT_SYSTEM_ERROR;
   } else {
     diagCallbacks.push_back(info);

     if (! Unlock( ))
       return VXIlog_RESULT_SYSTEM_ERROR;
   }

   return VXIlog_RESULT_SUCCESS;  
}


SBlogDiagCallbackData*
SBlog::UnregisterDiagnosticListener(SBlogDiagCallbackData *info)
{
  if (info == NULL) {
    Error(203, L"SBlog: Internal error in UnregisterDiagnosticListener(), "
	  L"NULL callback data", NULL);
    return NULL;
  }

  SBlogDiagCallbackData *status = NULL;
  if (! Lock( )) {
    return NULL;
  } else {
    for (DIAGCALLBACKS::iterator i = diagCallbacks.begin();
	 i != diagCallbacks.end(); ++i)
      {
	if (*info != *(*i)) continue;
	status = *i;
	diagCallbacks.erase(i);
	break;
      }

    if (! Unlock( ))
      return NULL;
  }

  return status; 
}


VXIlogResult SBlog::RegisterEventListener(SBlogEventCallbackData *info)
{
   if (info == NULL) {
     Error(204, L"SBlog: Internal error in RegisterEventListener(), NULL "
	   L"callback data", NULL);
     return VXIlog_RESULT_INVALID_ARGUMENT;
   }

   if (! Lock( )) {
     return VXIlog_RESULT_SYSTEM_ERROR;
   } else {
     eventCallbacks.push_back(info);

     if (! Unlock( ))
       return VXIlog_RESULT_SYSTEM_ERROR;
   }

   return VXIlog_RESULT_SUCCESS;  
}


SBlogEventCallbackData*
SBlog::UnregisterEventListener(SBlogEventCallbackData *info)
{
  if (info == NULL) {
    Error(205, L"SBlog: Internal error in UnregisterEventListener(), NULL "
	  L"callback data", NULL);
    return NULL;
  }

  SBlogEventCallbackData *status = NULL;
  if (! Lock( )) {
    return NULL;
  } else {
    for (EVENTCALLBACKS::iterator i = eventCallbacks.begin();
	 i != eventCallbacks.end(); ++i)
      {
	if (*info != *(*i)) continue;
	status = *i;
	eventCallbacks.erase(i);
	break;
      }
    
    if (! Unlock( ))
      return NULL;
  }

  return status; 
}


VXIlogResult SBlog::RegisterContentListener(SBlogContentCallbackData *info)
{
   if (info == NULL) {
     Error(206, L"SBlog: Internal error in RegisterContentListener(), NULL "
	  L"callback data", NULL);
     return VXIlog_RESULT_INVALID_ARGUMENT;
   }

   if (! Lock( )) {
     return VXIlog_RESULT_SYSTEM_ERROR;
   } else {
     contentCallbacks.push_back(info);

     if (! Unlock( ))
       return VXIlog_RESULT_SYSTEM_ERROR;
   }

   return VXIlog_RESULT_SUCCESS;  
}


SBlogContentCallbackData*
SBlog::UnregisterContentListener(SBlogContentCallbackData *info)
{
  if (info == NULL) {
    Error(207, L"SBlog: Internal error in RegisterContentListener(), NULL "
	  L"callback data", NULL);
    return NULL;
  }

  SBlogContentCallbackData *status = NULL;
  if (! Lock( )) {
    return NULL;
  } else {
    for (CONTENTCALLBACKS::iterator i = contentCallbacks.begin();
	 i != contentCallbacks.end(); ++i)
      {
	if (*info != *(*i)) continue;
	status = *i;
	contentCallbacks.erase(i);
	break;
      }
    
    if (! Unlock( ))
      return NULL;
  }

  return status; 
}


VXIlogResult SBlog::ParseKeyValue(const VXIchar *format,
                                  va_list args,
                                  VXIVector *keys,
                                  VXIVector *values) const
{
  const VXIchar SEP[] = L"{*}";
  const int SEP_LEN = 3;  
  if (( format == NULL ) || ( keys == NULL ) || ( values == NULL )) {
    Error(208, L"SBlog: Internal error in ParseKeyValue(), invalid argument",
	  NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  // Insert delimiters into a revised format string, this does
  // validation as well as letting us split it into key/values later
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  bool hadFreeformText = false;
  int replacementStart = -1, fieldCount = 0;
  size_t resultFormatLen = 0;
  VXIchar resultFormat[MAX_LOG_BUFFER];
  resultFormat[0] = L'\0';

  for (int i = 0; (format[i] != L'\0') && (rc == VXIlog_RESULT_SUCCESS); i++) {
    if (format[i] == '%') {
      if (replacementStart > -1)
	replacementStart = -1; // double %%
      else
	replacementStart = i;
    } else if ((replacementStart > -1) && (SBlogIsAlpha(format[i])) &&
	       (format[i] != L'l') && (format[i] != L'L') && 
	       (format[i] != L'h')) {
      if ((fieldCount % 2 == 0) && (format[i] != L's') && 
	  (format[i] != L'S')) {
	// Keys must be a %s or %S, truncate from here
	Error(301, L"SBlog: Invalid format string for VXIlog API call, "
	      L"replacements for key names must be %s", L"%s%s", L"format",
	      format);
	rc = VXIlog_RESULT_NON_FATAL_ERROR;
      } else {
	// Insert the replacement expression and the seperator
	size_t index = resultFormatLen;
	resultFormatLen += (i - replacementStart) + 1 + SEP_LEN;
	if (resultFormatLen < MAX_LOG_BUFFER) {
	  wcsncpy(&resultFormat[index], &format[replacementStart],
		  (i - replacementStart) + 1);
	  index += (i - replacementStart) + 1;
	  wcscpy(&resultFormat[index], SEP);
	} else {
	  // Overflow, truncate the format string from here
	  rc = VXIlog_RESULT_NON_FATAL_ERROR;
	}
	
	replacementStart = -1;
	fieldCount++;
      }
    } else if (replacementStart == -1) {
      // Shouldn't have free-form text, skip it. Proceeding allows us
      // to gracefully handle things like "%s0x%p".
      hadFreeformText = true;
    }
  }

  // if key/value is not even truncate the field and return an error,
  // but proceed with the other fields. If there was free form text,
  // we skipped it and return an error, but proceed with logging.
  if (fieldCount % 2 != 0) {
    Error(302, L"SBlog: Invalid format string for VXIlog API call, "
	  L"missing value for a key", L"%s%s", L"format", format);
    rc = VXIlog_RESULT_NON_FATAL_ERROR;
    fieldCount--;
  } else if (hadFreeformText) {
    Error(303, L"SBlog: Invalid format string for VXIlog API call, "
	  L"must be a list of format strings for key/value pairs", 
	  L"%s%s", L"format", format);
    rc = VXIlog_RESULT_NON_FATAL_ERROR;
  }

  // get the values
  VXIchar result[MAX_LOG_BUFFER];
  result[0] = L'\0';
  SBlogVswprintf(result, MAX_LOG_BUFFER-1, resultFormat, args);

  // parse the key/value based on the inserted separators
  long fieldNum = 0;
  const VXIchar *start = result, *end;
  while ((fieldNum < fieldCount) && ((end = wcsstr(start, SEP)) != NULL)) {
    // Insert into key/value vector
    fieldNum++;
    if (fieldNum % 2) // odd is key
      VXIVectorAddElement(keys,(VXIValue*)VXIStringCreateN(start,end-start));
    else
      VXIVectorAddElement(values,(VXIValue*)VXIStringCreateN(start,end-start));

    // Advance
    start = &end[SEP_LEN];
  }

  if (fieldNum != fieldCount) {
    // parse error
    Error(304, L"SBlog: Internal error, parse failure after key/value "
	  L"insertions", L"%s%s%s%s", L"format", format, L"result", result);
    rc = VXIlog_RESULT_NON_FATAL_ERROR;
  }

  return rc;
}


VXIlogResult SBlog::Error(VXIunsigned errorID, const VXIchar *errorIDText,
			  const VXIchar *format, ...) const
{
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  // Avoid recursively reporting logging errors
  if (_internalErrorLoggingThread == GetThreadID())
    return VXIlog_RESULT_FAILURE;

  if (VXItrdMutexLock(_internalErrorLoggingLock) != VXItrd_RESULT_SUCCESS) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    VXIthreadID *loggingThread = 
      const_cast<VXIthreadID *>(&_internalErrorLoggingThread);
    *loggingThread = GetThreadID();

    va_list args;
    va_start(args, format);
    rc = ErrorLog(MODULE_NAME, errorID, format, args);
    if ( rc != VXIlog_RESULT_SUCCESS )
      SBlogVLogErrorToConsole(MODULE_NAME, errorID, errorIDText, format, args);
    va_end(args);

    *loggingThread = (VXIthreadID) -1;

    if (VXItrdMutexUnlock(_internalErrorLoggingLock) != VXItrd_RESULT_SUCCESS)
      return VXIlog_RESULT_SYSTEM_ERROR;
  }

  return rc;
}


VXIlogResult SBlog::GlobalError(VXIunsigned errorID, 
				const VXIchar *errorIDText,
				const VXIchar *format, ...)
{
  va_list args;
  va_start(args, format);
  VXIlogResult rc = SBlogVLogErrorToConsole(MODULE_NAME, errorID, errorIDText,
					    format, args);
  va_end(args);
  return rc;
}


bool SBlog::DiagnosticIsEnabled(VXIunsigned tagID)
{
  VXIunsigned bindex, bpos;
  if (!Convert2Index(tagID, &bindex, &bpos))
    return false;

  // if this tag is not turned on, just return
  if (!testbit(_tagIDs[bindex],bpos))
    return false;

  return true;
}


VXIlogResult SBlog::DiagnosticLog(VXIunsigned     tagID,
                                  const VXIchar*  subtag,
                                  const VXIchar*  format,
                                  va_list arguments) const
{
  VXIunsigned bindex, bpos;
  if (!Convert2Index(tagID, &bindex, &bpos))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  // if this tag is not turned on, just return
  if (!testbit(_tagIDs[bindex],bpos))
    return VXIlog_RESULT_SUCCESS;

  time_t timestamp;
  VXIunsigned timestampMsec;
  SBlogGetTime (&timestamp, &timestampMsec);

  wchar_t printmsg[MAX_LOG_BUFFER];
  printmsg[0] = L'\0';
  if (format != NULL)
    SBlogVswprintf(printmsg, MAX_LOG_BUFFER-1, format, arguments);
  
  if (subtag == NULL)
    subtag = L"";
  
  // Get a temporary callback vector copy to traverse, allows
  // listeners to unregister from within the listener, also increases
  // parallelism by allowing multiple threads to be calling the
  // callbacks at once
  VXIlogResult rc = VXIlog_RESULT_FAILURE;
  if (! Lock( )) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    DIAGCALLBACKS tempDiag = diagCallbacks;

    if (! Unlock( ))
      return VXIlog_RESULT_SYSTEM_ERROR;
    
    for (DIAGCALLBACKS::iterator i = tempDiag.begin(); i != tempDiag.end();++i)
      {
	(*i)->callback(_pThis, tagID, subtag, timestamp, timestampMsec, 
		       printmsg, (*i)->userdata);
	rc = VXIlog_RESULT_SUCCESS;
      }
  }
  
  return rc;
}


VXIlogResult SBlog::EventLog(VXIunsigned     eventID,
                             const VXIchar*  format,
                             va_list args) const
{
  time_t timestamp;
  VXIunsigned timestampMsec;
  SBlogGetTime (&timestamp, &timestampMsec);

  // Create key/value pairs
  VXIVector *keys = VXIVectorCreate();
  VXIVector *values = VXIVectorCreate();
  if ((! keys) || (! values)) {
    Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
  
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if (format) {
    rc = ParseKeyValue(format, args, keys, values);
    if (rc < VXIlog_RESULT_SUCCESS) {
      VXIVectorDestroy(&keys);
      VXIVectorDestroy(&values);
      return rc;
    }
  }
  
  // Get a temporary callback vector copy to traverse, allows
  // listeners to unregister from within the listener, also increases
  // parallelism by allowing multiple threads to be calling the
  // callbacks at once
  if (rc == VXIlog_RESULT_SUCCESS)
    rc = VXIlog_RESULT_FAILURE;
  if (! Lock( )) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    EVENTCALLBACKS tempEvent = eventCallbacks;

    if (! Unlock( ))
      return VXIlog_RESULT_SYSTEM_ERROR;
  
    for (EVENTCALLBACKS::iterator i = tempEvent.begin();
	 i != tempEvent.end(); ++i)
      {
	(*i)->callback(_pThis, eventID, timestamp, timestampMsec, keys, values,
		       (*i)->userdata);
	if (rc == VXIlog_RESULT_FAILURE)
	  rc = VXIlog_RESULT_SUCCESS;
      }
  }
  
  VXIVectorDestroy(&keys);
  VXIVectorDestroy(&values);

  return rc;
}


VXIlogResult SBlog::EventLog(VXIunsigned      eventID,
			     const VXIVector* keys,
			     const VXIVector* values) const
{
  time_t timestamp;
  VXIunsigned timestampMsec;
  SBlogGetTime (&timestamp, &timestampMsec);

  // Get a temporary callback vector copy to traverse, allows
  // listeners to unregister from within the listener, also increases
  // parallelism by allowing multiple threads to be calling the
  // callbacks at once
  VXIlogResult rc = VXIlog_RESULT_FAILURE;
  if (! Lock( )) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    EVENTCALLBACKS tempEvent = eventCallbacks;

    if (! Unlock( ))
      return VXIlog_RESULT_SYSTEM_ERROR;
  
    for (EVENTCALLBACKS::iterator i = tempEvent.begin();
	 i != tempEvent.end(); ++i)
      {
	(*i)->callback(_pThis, eventID, timestamp, timestampMsec, keys, values,
		       (*i)->userdata);
	rc = VXIlog_RESULT_SUCCESS;
      }
  }

  return rc;
}


VXIlogResult SBlog::ErrorLog(const VXIchar*   moduleName,
                             VXIunsigned      errorID,
                             const VXIchar*   format,
                             va_list          args) const
{
  time_t  timestamp;
  VXIunsigned timestampMsec;
  SBlogGetTime (&timestamp, &timestampMsec);

  // Create key/value pairs
  VXIVector *keys = VXIVectorCreate();
  VXIVector *values = VXIVectorCreate();
  if ((! keys) || (! values)) {
    Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
  
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  if (format) {
    rc = ParseKeyValue(format, args, keys, values);
    if (rc < VXIlog_RESULT_SUCCESS) {
      VXIVectorDestroy(&keys);
      VXIVectorDestroy(&values);
      return rc;
    }
  }
  
  // Get a temporary callback vector copy to traverse, allows
  // listeners to unregister from within the listener, also increases
  // parallelism by allowing multiple threads to be calling the
  // callbacks at once
  if (rc == VXIlog_RESULT_SUCCESS)
    rc = VXIlog_RESULT_FAILURE;
  if (! Lock( )) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    ERRORCALLBACKS tempError = errorCallbacks;

    if (! Unlock( ))
      return VXIlog_RESULT_SYSTEM_ERROR; 

    for (ERRORCALLBACKS::iterator i = tempError.begin();
	 i != tempError.end(); ++i)
      {
	(*i)->callback(_pThis, 
		       (moduleName && moduleName[0] ? moduleName : L"UNKNOWN"),
		       errorID, timestamp, timestampMsec, keys, values, 
		       (*i)->userdata);
	if (rc == VXIlog_RESULT_FAILURE)
	  rc = VXIlog_RESULT_SUCCESS;
      }
  }
  
  VXIVectorDestroy(&keys);
  VXIVectorDestroy(&values); 

  // Want to warn the caller that NULL moduleName really isn't OK, but
  // logged it anyway
  if ((! moduleName) || (! moduleName[0])) {
    Error(305, L"SBlog: Empty module name passed to VXIlog API call", NULL);
    rc = VXIlog_RESULT_INVALID_ARGUMENT;
  }
  
  return rc;
}


VXIlogResult SBlog::ContentOpen(const VXIchar*      moduleName,
				const VXIchar*      contentType,
				VXIString**         logKey,
				VXIString**         logValue,
				VXIlogStream**      stream) const
{
  if ((! contentType) || (! contentType[0]) || (! logKey) || (! logValue) ||
      (! stream)) {
    Error(306, L"SBlog: Invalid argument to VXIlog::ContentOpen()", NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  *logKey = *logValue = NULL;
  *stream = NULL;

  const VXIchar *finalModuleName = L"UNKNOWN";
  if ((! moduleName) || (! moduleName[0])) {
    Error(305, L"SBlog: Empty module name passed to VXIlog API call", NULL);
  } else {
    finalModuleName = moduleName;
  }

  VXIlogResult rc = VXIlog_RESULT_FAILURE;

  // Get a temporary callback vector copy to traverse, allows
  // listeners to unregister from within the listener, also increases
  // parallelism by allowing multiple threads to be calling the
  // callbacks at once
  if (! Lock( )) {
    return VXIlog_RESULT_SYSTEM_ERROR;
  } else {
    CONTENTCALLBACKS tempContent = contentCallbacks;

    if (! Unlock( ))
      return VXIlog_RESULT_SYSTEM_ERROR; 

    VXIlogStream *finalStream = NULL;

    for (CONTENTCALLBACKS::iterator i = tempContent.begin();
        i != tempContent.end(); ++i)
    {
      VXIString *logKey_    = NULL;
      VXIString *logValue_  = NULL;
      SBlogStream *stream_  = NULL;

      VXIlogResult rc2 = (*i)->callback(_pThis, 
					finalModuleName,
					contentType, 
					(*i)->userdata,
					&logKey_,
					&logValue_,
					&stream_);
      if ((rc2 == VXIlog_RESULT_SUCCESS) && (stream_)) {
	// Return the key and value of only the first listener
	if ( rc != VXIlog_RESULT_SUCCESS ) {
	  finalStream = new VXIlogStream;
	  if (! finalStream) {
	    VXIStringDestroy(&logKey_);       
	    VXIStringDestroy(&logValue_);
	    stream_->Close(&stream_);
	    Error(SBLOG_ERR_OUT_OF_MEMORY);
	    return VXIlog_RESULT_OUT_OF_MEMORY;
	  }

	  rc = VXIlog_RESULT_SUCCESS;
	  *logKey = logKey_;
	  *logValue = logValue_;
	  *stream = (VXIlogStream *) finalStream;
	} else {
	  VXIStringDestroy(&logKey_);       
	  VXIStringDestroy(&logValue_);
	}

	// Store the stream
	finalStream->streams.push_back(stream_);
      } else if ((rc != VXIlog_RESULT_SUCCESS) &&
		 (((rc < 0) && (rc2 < rc)) ||
		  ((rc > 0) && (rc2 > rc)))) {
	// Return success if any listener returns success but keep the
	// worst error otherwise
	rc = rc2;
      }
    }
  }

  return rc;
}


VXIlogResult SBlog::ContentClose(VXIlogStream**     stream) const
{
  if ((! stream) || (! *stream)) {
    Error(307, L"SBlog: Invalid argument to VXIlog::ContentClose()", NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;

  // Close each of the underlying listener streams
  for (VXIlogStream::STREAMS::iterator vi = (*stream)->streams.begin( );
       vi != (*stream)->streams.end( ); vi++) {
    SBlogStream *s = *vi;
    VXIlogResult rc2 = s->Close (&s);
    if ((rc == VXIlog_RESULT_SUCCESS) || ((rc < 0) && (rc2 < rc)) ||
	((rc > 0) && (rc2 > rc)))
      rc = rc2;
  }

  delete *stream;
  *stream = NULL; 
  return rc;
}


VXIlogResult SBlog::ContentWrite(const VXIbyte*     buffer,
				 VXIulong           buflen,
				 VXIulong*          nwritten,
				 VXIlogStream*      stream) const
{
  if ((! buffer) || (buflen < 1) || (! nwritten) || (! stream)) {
    Error(308, L"SBlog: Invalid argument to VXIlog::ContentWrite()", NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }
  
  VXIlogResult rc = VXIlog_RESULT_SUCCESS;
  *nwritten = 0;

  // Write to each of the underlying listener streams
  for (VXIlogStream::STREAMS::iterator vi = stream->streams.begin( );
       vi != stream->streams.end( ); vi++) {
    SBlogStream *s = *vi;
    VXIulong nw = 0;
    VXIlogResult rc2 = s->Write (s, buffer, buflen, &nw);
    if (rc == VXIlog_RESULT_SUCCESS) {
      if (nw > *nwritten)
	*nwritten = nw;
    } else if ((rc == VXIlog_RESULT_SUCCESS) || ((rc < 0) && (rc2 < rc)) ||
	       ((rc > 0) && (rc2 > rc))) {
      rc = rc2;
    }
  }

  return rc;
}

  
/***********************************************************************/

VXIlogResult SBlogControlDiagnosticTag(SBlogInterface *pThis,
                                       VXIunsigned tagID,
                                       VXIbool     state)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  return (me->ControlDiagnosticTag(tagID, state));
}


VXIlogResult SBlogRegisterErrorListener(SBlogInterface *pThis,
                                        SBlogErrorListener* alistener,
                                        void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogErrorCallbackData *info = new SBlogErrorCallbackData;
  if (info == NULL) {
    me->Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
 
  info->callback = alistener;
  info->userdata = userdata;
  return ( me->RegisterErrorListener(info));
} 


VXIlogResult SBlogUnregisterErrorListener(SBlogInterface *pThis,
                                          SBlogErrorListener* alistener,
                                          void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogErrorCallbackData info = { alistener, userdata };
  SBlogErrorCallbackData *p = me->UnregisterErrorListener(&info);
  if (p) delete p;
  return VXIlog_RESULT_SUCCESS;
} 


VXIlogResult SBlogRegisterDiagnosticListener(SBlogInterface *pThis,
                                             SBlogDiagnosticListener*alistener,
                                             void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogDiagCallbackData *info = new SBlogDiagCallbackData;
  if (info == NULL) {
    me->Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
 
  info->callback = alistener;
  info->userdata = userdata;
  return ( me->RegisterDiagnosticListener(info));
} 

VXIlogResult SBlogUnregisterDiagnosticListener(
                                     SBlogInterface *pThis,
                                     SBlogDiagnosticListener* alistener,
                                     void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogDiagCallbackData info = { alistener, userdata };
  SBlogDiagCallbackData *p = me->UnregisterDiagnosticListener(&info);
  if (p) delete p;

  return VXIlog_RESULT_SUCCESS;
} 


VXIlogResult SBlogRegisterEventListener(SBlogInterface *pThis,
                                        SBlogEventListener* alistener,
                                        void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;
  
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogEventCallbackData *info = new SBlogEventCallbackData;
  if (info == NULL) {
    me->Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
 
  info->callback = alistener;
  info->userdata = userdata;
  return ( me->RegisterEventListener(info));
} 


VXIlogResult SBlogUnregisterEventListener(SBlogInterface *pThis,
                                          SBlogEventListener* alistener,
                                          void*  userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogEventCallbackData info = { alistener, userdata };
  SBlogEventCallbackData *p = me->UnregisterEventListener(&info);
  if (p) delete p;

  return VXIlog_RESULT_SUCCESS;
} 


VXIlogResult SBlogRegisterContentListener(SBlogInterface *pThis,
					  SBlogContentListener* alistener,
					  void* userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;
  
  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogContentCallbackData *info = new SBlogContentCallbackData;
  if (info == NULL) {
    me->Error(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
 
  info->callback = alistener;
  info->userdata = userdata;
  return ( me->RegisterContentListener(info));
} 


VXIlogResult SBlogUnregisterContentListener(SBlogInterface *pThis,
					    SBlogContentListener* alistener,
					    void*  userdata)
{
  if ((pThis == NULL) || (alistener == NULL))
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI *) pThis; 
  SBlog *me = (SBlog *)temp->impl_;
  SBlogContentCallbackData info = { alistener, userdata };
  SBlogContentCallbackData *p = me->UnregisterContentListener(&info);
  if (p) delete p;

  return VXIlog_RESULT_SUCCESS;
} 

/**********************************************************************
*/

SBLOG_API VXIint32 SBlogGetVersion(void)
{
  return VXI_CURRENT_VERSION;
}


SBLOG_API const VXIchar* SBlogGetImplementationName(void)
{
  static const VXIchar IMPLEMENTATION_NAME[] = COMPANY_DOMAIN L".SBlog";
  return IMPLEMENTATION_NAME;
}


SBLOG_API VXIbool SBlogDiagnosticIsEnabled(VXIlogInterface * pThis,
                                           VXIunsigned tagID)
{
  if (pThis == NULL)
    return FALSE;

  myAPI *temp = (myAPI *) pThis;
  SBlog *me = (SBlog *) temp->impl_;

  if (!me->DiagnosticIsEnabled(tagID)) return FALSE;
  return TRUE;
}


SBLOG_API VXIlogResult SBlogDiagnostic(VXIlogInterface*        pThis,
                                       VXIunsigned             tagID,
                                       const VXIchar*          subtag,
                                       const VXIchar*          format,
                                       ...)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI *) pThis;
  SBlog *me = (SBlog *) temp->impl_;
  va_list args;
  va_start(args, format);

  VXIlogResult ret = me->DiagnosticLog(tagID, subtag, format, args);
  va_end(args);

  return ret;
}

SBLOG_API VXIlogResult SBlogVDiagnostic(VXIlogInterface*        pThis,
                                        VXIunsigned             tagID,
                                        const VXIchar*          subtag,
                                        const VXIchar*          format,
                                        va_list                 vargs)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI *)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->DiagnosticLog(tagID, subtag, format, vargs);

  return ret;
}


SBLOG_API VXIlogResult SBlogEvent(VXIlogInterface*        pThis,
                                  VXIunsigned             eventID,
                                  const VXIchar*          format,
                                  ...)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  va_list args;
  va_start(args, format);
  VXIlogResult ret = me->EventLog(eventID, format, args);
  va_end(args);

  return ret;
}


SBLOG_API VXIlogResult SBlogVEvent(VXIlogInterface*        pThis,
                                   VXIunsigned             eventID,
                                   const VXIchar*          format,
                                   va_list                 vargs)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->EventLog(eventID, format, vargs);

  return ret;
}


SBLOG_API VXIlogResult SBlogEventVector(VXIlogInterface*        pThis,
					VXIunsigned             eventID,
					const VXIVector*        keys,
					const VXIVector*        values)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->EventLog(eventID, keys, values);

  return ret;
}


SBLOG_API VXIlogResult SBlogError(VXIlogInterface*        pThis,
                                  const VXIchar*          moduleName,
                                  VXIunsigned             errorID,
                                  const VXIchar*          format,
                                  ...)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  va_list args;
  va_start(args, format);
  VXIlogResult ret = me->ErrorLog(moduleName, errorID, format, args);
  va_end(args);

  return ret;
}


SBLOG_API VXIlogResult SBlogVError(VXIlogInterface*        pThis,
                                   const VXIchar*          moduleName,
                                   VXIunsigned             errorID,
                                   const VXIchar*          format,
                                   va_list                 vargs)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->ErrorLog(moduleName, errorID, format, vargs);

  return ret;
}


SBLOG_API VXIlogResult SBlogContentOpen(VXIlogInterface*  pThis,
					const VXIchar*    moduleName,
					const VXIchar*    contentType,
					VXIString**       logKey,
					VXIString**       logValue,
					VXIlogStream**    stream)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->ContentOpen(moduleName, contentType,
				     logKey, logValue, stream);

  return ret;
}


SBLOG_API VXIlogResult SBlogContentClose(VXIlogInterface* pThis,
					 VXIlogStream**   stream)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->ContentClose(stream);

  return ret;
}


SBLOG_API VXIlogResult SBlogContentWrite(VXIlogInterface* pThis,
					 const VXIbyte*   buffer,
					 VXIulong         buflen,
					 VXIulong*        nwritten,
					 VXIlogStream*    stream)
{
  if (pThis == NULL)
    return VXIlog_RESULT_INVALID_ARGUMENT;

  myAPI *temp = (myAPI*)pThis;
  SBlog *me = (SBlog *)temp->impl_;
  VXIlogResult ret = me->ContentWrite(buffer, buflen, nwritten, stream);

  return ret;
}


// -----1=0-------2=0-------3=0-------4=0-------5=0-------6=0-------7=0-------8

/**
 * Global platform initialization of SBlog
 *
 * @result VXIlogResult 0 on success
 */
SBLOG_API VXIlogResult SBlogInit(void)
{
  if (gblInitialized == true) {
    SBlog::GlobalError(SBLOG_ERR_ALREADY_INITIALIZED);
    return VXIlog_RESULT_FATAL_ERROR;
  }

  gblInitialized = true;
  return VXIlog_RESULT_SUCCESS;
}


/**
 * Global platform shutdown of Log
 *
 * @result VXIlogResult 0 on success
 */
SBLOG_API VXIlogResult SBlogShutDown(void)
{
  if (gblInitialized == false) {
    SBlog::GlobalError(SBLOG_ERR_NOT_INITIALIZED);
    return VXIlog_RESULT_FATAL_ERROR;
  }

  gblInitialized = false;
  return VXIlog_RESULT_SUCCESS;
}


/**
 * Create a new log service handle
 *
 * @result VXIlogResult 0 on success 
 */
SBLOG_API VXIlogResult SBlogCreateResource(VXIlogInterface **log)
{
  if (gblInitialized == false) {
    SBlog::GlobalError(SBLOG_ERR_NOT_INITIALIZED);
    return VXIlog_RESULT_FATAL_ERROR;
  }

  if (log == NULL) {
    SBlog::GlobalError(309, L"SBlog: SBlogCreateResource() requires a "
		       L"non-NULL log interface pointer", NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  myAPI *temp = new myAPI;
  if (temp == NULL) {
    SBlog::GlobalError(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
  memset (temp, 0, sizeof (myAPI));
  
  SBlog *me = new SBlog(&temp->intf);
  if (me == NULL) {
    delete temp;
    SBlog::GlobalError(SBLOG_ERR_OUT_OF_MEMORY);
    return VXIlog_RESULT_OUT_OF_MEMORY;
  }
  
  // Initialize the VXIlogInterface function pointers
  temp->intf.vxilog.GetVersion            = SBlogGetVersion;
  temp->intf.vxilog.GetImplementationName = SBlogGetImplementationName;
  temp->intf.vxilog.Error                 = SBlogError;
  temp->intf.vxilog.VError                = SBlogVError;
  temp->intf.vxilog.Diagnostic            = SBlogDiagnostic;
  temp->intf.vxilog.VDiagnostic           = SBlogVDiagnostic;
  temp->intf.vxilog.DiagnosticIsEnabled   = SBlogDiagnosticIsEnabled;
  temp->intf.vxilog.Event                 = SBlogEvent;
  temp->intf.vxilog.VEvent                = SBlogVEvent;
  temp->intf.vxilog.EventVector           = SBlogEventVector;
  temp->intf.vxilog.ContentOpen           = SBlogContentOpen;
  temp->intf.vxilog.ContentClose          = SBlogContentClose;
  temp->intf.vxilog.ContentWrite          = SBlogContentWrite;
  
  // Initialize the SBlogInterface functions
  temp->intf.RegisterErrorListener = SBlogRegisterErrorListener;
  temp->intf.UnregisterErrorListener = SBlogUnregisterErrorListener;
  temp->intf.RegisterDiagnosticListener = SBlogRegisterDiagnosticListener;
  temp->intf.UnregisterDiagnosticListener = SBlogUnregisterDiagnosticListener;
  temp->intf.RegisterEventListener = SBlogRegisterEventListener;
  temp->intf.UnregisterEventListener = SBlogUnregisterEventListener;
  temp->intf.RegisterContentListener = SBlogRegisterContentListener;
  temp->intf.UnregisterContentListener = SBlogUnregisterContentListener;
  temp->intf.ControlDiagnosticTag = SBlogControlDiagnosticTag;
  temp->impl_ = (void*)me;

  // Return the object
  *log = &temp->intf.vxilog;
  return VXIlog_RESULT_SUCCESS;
}


/**
 * Destroy the interface and free internal resources
 *
 * @result VXIlogResult 0 on success 
 */
SBLOG_API VXIlogResult SBlogDestroyResource(VXIlogInterface **log)
{
  if (gblInitialized == false) {
    SBlog::GlobalError(SBLOG_ERR_NOT_INITIALIZED);
    return VXIlog_RESULT_FATAL_ERROR;
  }

  if ((log == NULL) || (*log == NULL)) {
    SBlog::GlobalError(310, L"SBlog: SBlogDestroyResource() requires a "
		       L"non-NULL log interface pointer", NULL);
    return VXIlog_RESULT_INVALID_ARGUMENT;
  }

  // Delete the object
  myAPI *temp = (myAPI *) *log;
  SBlog *me = (SBlog *)temp->impl_;
  if (me) delete me;
  if (temp) delete temp;
  *log = NULL;

  return VXIlog_RESULT_SUCCESS;
}

